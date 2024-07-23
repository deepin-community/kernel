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

#include "util.h"
char func_name[255] = {'\0'};

unsigned int util_crc32(unsigned char *pBuf, unsigned int nSize)
{
    static unsigned int s_u32CrcTables[256];
    static unsigned int  s_bInited = 0;
    unsigned int crc;
    const unsigned char *p;

    if (!s_bInited)
    {
        unsigned int poly = 0x04C11DB7;
        unsigned int i;
        unsigned int j;

        for (i = 0; i < 256; i++)
        {
            crc = i << 24;
            for (j = 0; j < 8; j++)
            {
                crc = (crc << 1) ^ ((crc & 0x80000000) ? (poly) : 0);
            }
            s_u32CrcTables[i] = crc;
        }

        s_bInited =1;
    }

    crc = 0xFFFFFFFF;
    for (p = pBuf; p < pBuf + nSize; ++p)
    {
        crc = (crc << 8) ^ s_u32CrcTables[((crc >> 24) ^ *p) & 0xFF];
    }

    return crc;
}

unsigned int util_log2(unsigned int s)
{
    unsigned int   iter = (unsigned int)-1;
    switch (s)
    {
    case 1:
        iter = 0;
        break;
    case 2:
        iter = 1;
        break;
    case 4:
        iter = 2;
        break;
    case 8:
        iter = 3;
        break;
    case 16:
        iter = 4;
        break;
    case 32:
        iter = 5;
        break;
    case 64:
        iter = 6;
        break;
    case 128:
        iter = 7;
        break;
    case 256:
        iter = 8;
        break;
    case 512:
        iter = 9;
        break;
    case 1024:
        iter = 10;
        break;
    default:
        {
            unsigned int   d = 1;
            do {
                d *= 2;
                iter++;
            } while (d < s);
            iter += ((s << 1) != d);
        }
    }
    return iter;
}


/*
** buffer: memory to dump
** size  : dumped size in DWORD
*/

void util_dump_memory(struct os_printer *p, void *buffer, unsigned int size, char *msg)
{
    unsigned int dword = size >> 2;
    unsigned int row   = dword >> 2;
    unsigned int left  = dword%4;
    unsigned int *data = buffer;
    unsigned int i;
    unsigned int offset = 0;

    if((size == 0) || (buffer == NULL)) return;

    gf_printf(p, "<< ------ Start dump %s - Size: 0x%08x, dword:0x%x ------>>\n", msg, size, dword);

    for(i = 0; i < row; i++)
    {
        gf_printf(p,"#0x%08x %08x, %08x, %08x, %08x\n", offset, data[0], data[1], data[2], data[3]);
        data = data + 4;
        offset += 0x10;
    }

    if(left == 3)
    {
        gf_printf(p,"#0x%08x %08x, %08x, %08x\n", offset, data[0], data[1], data[2]);
    }
    else if(left == 2)
    {
        gf_printf(p,"#0x%08x %08x, %08x\n", offset, data[0], data[1]);
    }
    else if(left == 1)
    {
        gf_printf(p,"#0x%08x %08x\n", offset, data[0]);
    }

    gf_printf(p,"<< ----------- End dump %s ---------------->>\n", msg);
}

/*
 * buffer: memory to dump
 * size  : dumped size in byte
 */
void util_dump_raw_memory_to_file(void *buffer, unsigned int size, char *filename)
{
    struct os_file *file = gf_file_open(filename, OS_WRONLY | OS_CREAT, 0644);

    if(file == NULL)
    {
        return;
    }

    if(buffer != NULL && size != 0)
    {
        gf_file_write(file, buffer, size);
    }

    gf_file_close(file);
}

void util_dump_memory_to_file(void *buffer, unsigned int size, char *private_msg, char *filename)
{
    struct os_file *file = gf_file_open(filename, OS_WRONLY | OS_CREAT | OS_APPEND, 0644);
    int    len           = gf_strlen(private_msg);
    char   *msg          = gf_calloc(256);

    if(file == NULL)
    {
        gf_free(msg);
        return;
    }

    if(len > 0)
    {
        gf_file_write(file, private_msg, len);
    }

    if(buffer != NULL && size != 0)
    {
        unsigned int dword = size >> 2;
        unsigned int row   = dword >> 2;
        unsigned int left  = dword%4;
        unsigned int *data = buffer;
        unsigned int i;

        gf_vsnprintf(msg, 256, "-------------------->\nmemory_dump: 0x%p -> 0x%p, size:0x%x.\n", buffer, (char*)buffer+size, size);
        gf_file_write(file, msg, gf_strlen(msg));

        gf_vsnprintf(msg, 256, "dword: 0x%x, row:0x%x, left:0x%x.\n", dword, row, left);
        gf_file_write(file, msg, gf_strlen(msg));
        for(i = 0; i < row; i++)
        {
            gf_vsnprintf(msg, 256, "#%p %08x, %08x, %08x, %08x\n", data, data[0], data[1], data[2], data[3]);
            gf_file_write(file, msg, gf_strlen(msg));
            data += 4;
        }

        if(left == 3)
        {
            gf_vsnprintf(msg, 256, "#%p %08x, %08x, %08x\n", data, data[0], data[1], data[2]);
            gf_file_write(file, msg, gf_strlen(msg));
        }
        else if(left == 2)
        {
            gf_vsnprintf(msg, 256, "#%p %08x, %08x\n", data, data[0], data[1], data[2]);
            gf_file_write(file, msg, gf_strlen(msg));
        }
        else if(left == 1)
        {
            gf_vsnprintf(msg, 256, "#%p %08x\n", data, data[0], data[1], data[2]);
            gf_file_write(file, msg, gf_strlen(msg));
        }
        gf_vsnprintf(msg, 256, "-------------------->");
        gf_file_write(file, msg, gf_strlen(msg));
    }

    gf_free(msg);
    gf_file_close(file);

}

void util_write_to_file(char *buffer, unsigned int size, char *private_msg, char *filename)
{
    struct os_file *file = gf_file_open(filename, OS_WRONLY | OS_CREAT | OS_APPEND, 0644);
    int    len           = gf_strlen(private_msg);

    if(file == NULL)
    {
        return;
    }

    if(len > 0)
    {
        gf_file_write(file, private_msg, len);
    }

    if(buffer != NULL && size != 0)
    {
        gf_file_write(file, buffer, size);
    }

    gf_file_close(file);
}

long util_get_ptr_span_trace(void *start, void *end, const char *func, int line)
{
    long span = (char*)start - (char*)end;

#ifdef _DEBUG_
    if(span < 0)
    {
        gf_error("span calc failed: start:%p, end:%p, func:%s, line:%d.\n",
            start, end, func, line);
    }
#endif
    return span;
}

static int util_event_thread(void *data)
{
    util_event_thread_t *thread = data;
    gf_event_status_t ret;

    gf_set_freezable();

    do
    {
        ret = gf_thread_wait(thread->event, thread->timeout_msec);

        thread->event_handler(thread->private_data, ret);

        if(thread->can_freeze)
        {
            gf_try_to_freeze();
        }

    } while(!gf_thread_should_stop());

    return 0;
}


struct general_wait_event*  util_create_event(int  need_queue)
{
    struct general_wait_event     *event = gf_calloc(sizeof(struct general_wait_event));

    if(event)
    {
        event->need_queue = need_queue;
        event->event = 0;
        event->event_lock = gf_create_spinlock(0);

        return event;
    }

    return NULL;
}

util_event_thread_t *util_create_event_thread(util_event_handler_t handler, void *private_data, const char *thread_name, int msec)
{
    util_event_thread_t *thread = gf_calloc(sizeof(util_event_thread_t));

    thread->timeout_msec  = msec;
    thread->can_freeze    = TRUE; //default freeze value, if want freeze control by thread, adjust this value in thread handler
    thread->event_handler = handler;
    thread->private_data  = private_data;

    thread->event         = gf_create_event(0);
    thread->os_thread     = gf_create_thread(util_event_thread, thread, thread_name);

    return thread;
}

void util_destroy_event_thread(util_event_thread_t *thread)
{
    gf_destroy_thread(thread->os_thread);
    gf_destroy_event(thread->event);

    gf_free(thread);
}

void util_wakeup_event_thread(util_event_thread_t *thread)
{
    gf_thread_wake_up(thread->event);
}


#if ENABLE_UTIL_LOG

#include <stdarg.h>

#define MAX_LINE_NUM   512
#define MAX_STR_LINGTH 256

static struct os_mutex *util_log_lock = NULL;
static char util_log_str[MAX_LINE_NUM][MAX_STR_LINGTH];
static int  util_log_id = 0;

void util_init_log(void)
{
    int i;
    util_log_lock = gf_create_mutex();

    for(i = 0; i < MAX_LINE_NUM; i++)
    {
        gf_memset(util_log_str[i], 0, MAX_STR_LINGTH);
    }
}

#include <linux/kernel.h>

void util_log(const char *fmt, ...)
{
    char     buffer[MAX_STR_LINGTH];
    int      index = 0;
    va_list  args;

    va_start(args, fmt);

    vsnprintf(buffer, sizeof(buffer), fmt, args);

    va_end(args);

    gf_mutex_lock(util_log_lock);

    index = (util_log_id % MAX_LINE_NUM);

    gf_vsnprintf(util_log_str[index], MAX_STR_LINGTH-1, "%8x: %s", util_log_id, buffer);

    util_log_id++;

    gf_mutex_unlock(util_log_lock);
}

void util_print_log(void)
{
    int i = 0;

    gf_mutex_lock(util_log_lock);

    gf_info("CURR_LOG_ID: %x.\n", util_log_id);

    for(i = 0; i < MAX_LINE_NUM; i++)
    {
        gf_info("%s", util_log_str[i]);
    }

    gf_mutex_unlock(util_log_lock);
}

#endif
