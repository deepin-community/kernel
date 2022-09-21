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

#ifndef __GF_TRACE_H__
#define __GF_TRACE_H__

#ifdef GF_USERMODE_TRACE

#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>
#include <sys/cdefs.h>
#include <sys/types.h>
#include <unistd.h>

#define CC_UNLIKELY( exp )  (__builtin_expect( !!(exp), false ))

/*
 default use c++11 <atomic> or C11 <stdatomic.h> in this header,
 but if the compiler is c++ without -std=c++11, which has no <atomic>,
 and g++ is incompatible with <stdatomic.h>, we should not use atomic
 in this header, or compile errors will happen.
 in this case, use extern gf_trace_init() instead of inline.
*/
#ifndef USE_ATOMIC_IN_TRACE_H
#define USE_ATOMIC_IN_TRACE_H 1
#endif

#ifdef __cplusplus
#if __cplusplus < 201103L
#undef USE_ATOMIC_IN_TRACE_H
#define USE_ATOMIC_IN_TRACE_H 0
#else /* __cplusplus < 201103L */
#include <atomic>
using std::atomic_bool;
using std::memory_order_acquire;
#endif
#else /* __cplusplus */
#include <stdatomic.h>
#endif

__BEGIN_DECLS

/**
 * The GF_TRACE_TAG macro can be defined before including this header to trace
 * using one of the tags defined below.  It must be defined to one of the
 * following GF_TRACE_TAG_* macros.  The trace tag is used to filter tracing in
 * userland to avoid some of the runtime cost of tracing when it is not desired.
 *
 * Defining GF_TRACE_TAG to be GF_TRACE_TAG_ALWAYS will result in the tracing always
 * being enabled - this should ONLY be done for debug code, as userland tracing
 * has a performance cost even when the trace is not being recorded.  Defining
 * GF_TRACE_TAG to be GF_TRACE_TAG_NEVER or leaving GF_TRACE_TAG undefined will result
 * in the tracing always being disabled.
 *
 * Keep these in sync with catapult/systrace/systrace/tracing_agents/ftrace_agent.py.
 */
#define GF_TRACE_TAG_NEVER            0       // This tag is never enabled.
#define GF_TRACE_TAG_ALWAYS           (1<<0)  // This tag is always enabled.
#define GF_TRACE_TAG_KEINTERFACE      (1<<1)
#define GF_TRACE_TAG_XSERVER_DRIVER   (1<<2)
#define GF_TRACE_TAG_CIL2_SERVICE     (1<<3)
#define GF_TRACE_TAG_DP_SERVER        (1<<4)
#define GF_TRACE_TAG_DP_BRIDGES       (1<<5)
#define GF_TRACE_TAG_GLCORE           (1<<6)
#define GF_TRACE_TAG_CLCORE           (1<<7)
#define GF_TRACE_TAG_EGL              (1<<8)
#define GF_TRACE_TAG_VPM              (1<<9)
#define GF_TRACE_TAG_VA               (1<<10)
#define GF_TRACE_TAG_VDPAU            (1<<11)
#define GF_TRACE_TAG_LAST             GF_TRACE_TAG_VDPAU

// Reserved for initialization.
#define GF_TRACE_TAG_NOT_READY        (1ULL<<63)

#define GF_TRACE_TAG_VALID_MASK ((GF_TRACE_TAG_LAST - 1) | GF_TRACE_TAG_LAST)

#ifndef GF_TRACE_TAG
#define GF_TRACE_TAG GF_TRACE_TAG_NEVER
#elif GF_TRACE_TAG > GF_TRACE_TAG_VALID_MASK
#error GF_TRACE_TAG must be defined to be one of the tags defined in cutils/trace.h
#endif

/**
 * Opens the trace file for writing and reads the property for initial tags.
 * The /sys/kernel/debug/dri/0/gf/umd_trace sets the tags to trace.
 * This function should not be explicitly called, the first call to any normal
 * trace function will cause it to be run safely.
 */
void gf_trace_setup();

/**
 * If tracing is ready, set gf_trace_enabled_tags to the debugfs file:
 * /sys/kernel/debug/dri/0/gf/umd_trace. Can be used as a tags change callback.
 */
void gf_trace_update_tags();

/**
 * Flag indicating whether setup has been completed, initialized to 0.
 * Nonzero indicates setup has completed.
 * Note: This does NOT indicate whether or not setup was successful.
 */
#if USE_ATOMIC_IN_TRACE_H
extern atomic_bool gf_trace_is_ready;
#endif

/**
 * Set of GF_TRACE_TAG flags to trace for, initialized to GF_TRACE_TAG_NOT_READY.
 * A value of zero indicates setup has failed.
 * Any other nonzero value indicates setup has succeeded, and tracing is on.
 */
extern uint64_t gf_trace_enabled_tags;

/**
 * Handle to the kernel's trace buffer, initialized to -1.
 * Any other value indicates setup has succeeded, and is a valid fd for tracing.
 */
extern int gf_trace_marker_fd;

/**
 * gf_trace_init readies the process for tracing by opening the trace_marker file.
 * Calling any trace function causes this to be run, so calling it is optional.
 * This can be explicitly run to avoid setup delay on first trace function.
 */
#define GF_TRACE_INIT() gf_trace_init()
#if USE_ATOMIC_IN_TRACE_H
static inline void gf_trace_init()
{
    if (CC_UNLIKELY(!atomic_load_explicit(&gf_trace_is_ready, memory_order_acquire))) {
        gf_trace_setup();
    }
}
#else
extern void gf_trace_init();
#endif

/**
 * Get the mask of all tags currently enabled.
 * It can be used as a guard condition around more expensive trace calculations.
 * Every trace function calls this, which ensures gf_trace_init is run.
 */
#define GF_TRACE_GET_ENABLED_TAGS() gf_trace_get_enabled_tags()
static inline uint64_t gf_trace_get_enabled_tags()
{
    gf_trace_init();
    return gf_trace_enabled_tags;
}

/**
 * Test if a given tag is currently enabled.
 * Returns nonzero if the tag is enabled, otherwise zero.
 * It can be used as a guard condition around more expensive trace calculations.
 */
#define GF_TRACE_ENABLED() gf_trace_is_tag_enabled(GF_TRACE_TAG)
static inline uint64_t gf_trace_is_tag_enabled(uint64_t tag)
{
    return gf_trace_get_enabled_tags() & tag;
}

/**
 * Trace the beginning of a context.  name is used to identify the context.
 * Optional args can be appended after a '|', args will be added to the slice.
 * This is often used to time function execution.
 */
#define GF_TRACE_BEGIN(name, ...) gf_trace_begin(GF_TRACE_TAG, name, ##__VA_ARGS__)
static inline void gf_trace_begin(uint64_t tag, const char* name, ...)
{
    if (CC_UNLIKELY(gf_trace_is_tag_enabled(tag))) {
        void gf_trace_begin_body(const char*, va_list);
        va_list args;
        va_start(args, name);
        gf_trace_begin_body(name, args);
        va_end(args);
    }
}

/**
 * Trace additional args of a context.
 * Some args are only known in the middle or end of the execution.
 * This should match up (and occur after) a corresponding GF_TRACE_BEGIN.
 */
#define GF_TRACE_ARGS(format, ...) gf_trace_args(GF_TRACE_TAG, format, ##__VA_ARGS__)
static inline void gf_trace_args(uint64_t tag, const char* format, ...)
{
    if (CC_UNLIKELY(gf_trace_is_tag_enabled(tag))) {
        void gf_trace_args_body(const char*, va_list);
        va_list args;
        va_start(args, format);
        gf_trace_args_body(format, args);
        va_end(args);
    }
}

/**
 * Trace the end of a context.
 * This should match up (and occur after) a corresponding GF_TRACE_BEGIN.
 */
#define GF_TRACE_END() gf_trace_end(GF_TRACE_TAG)
static inline void gf_trace_end(uint64_t tag)
{
    if (CC_UNLIKELY(gf_trace_is_tag_enabled(tag))) {
        void gf_trace_end_body();
        gf_trace_end_body();
    }
}

/**
 * Trace the end of a context, with extra args.
 * Some args are only known in the middle or end of the execution.
 * This should match up (and occur after) a corresponding GF_TRACE_BEGIN.
 */
#define GF_TRACE_END2(format, ...) gf_trace_end2(GF_TRACE_TAG, format, ##__VA_ARGS__)
static inline void gf_trace_end2(uint64_t tag, const char* format, ...)
{
    if (CC_UNLIKELY(gf_trace_is_tag_enabled(tag))) {
        void gf_trace_end2_body(const char*, va_list);
        va_list args;
        va_start(args, format);
        gf_trace_end2_body(format, args);
        va_end(args);
    }
}

/**
 * Trace the beginning of an asynchronous event. Unlike GF_TRACE_BEGIN/GF_TRACE_END
 * contexts, asynchronous events do not need to be nested. The name describes
 * the event, and the cookie provides a unique identifier for distinguishing
 * simultaneous events. The name and cookie used to begin an event must be
 * used to end it.
 */
#define GF_TRACE_ASYNC_BEGIN(name, cookie) \
    gf_trace_async_begin(GF_TRACE_TAG, name, cookie)
static inline void gf_trace_async_begin(uint64_t tag, const char* name,
        int32_t cookie)
{
    if (CC_UNLIKELY(gf_trace_is_tag_enabled(tag))) {
        void gf_trace_async_begin_body(const char*, int32_t);
        gf_trace_async_begin_body(name, cookie);
    }
}

/**
 * Trace the end of an asynchronous event.
 * This should have a corresponding GF_TRACE_ASYNC_BEGIN.
 */
#define GF_TRACE_ASYNC_END(name, cookie) gf_trace_async_end(GF_TRACE_TAG, name, cookie)
static inline void gf_trace_async_end(uint64_t tag, const char* name, int32_t cookie)
{
    if (CC_UNLIKELY(gf_trace_is_tag_enabled(tag))) {
        void gf_trace_async_end_body(const char*, int32_t);
        gf_trace_async_end_body(name, cookie);
    }
}

/**
 * Traces an integer counter value.  name is used to identify the counter.
 * This can be used to track how a value changes over time.
 */
#define GF_TRACE_INT(name, value) gf_trace_int(GF_TRACE_TAG, name, value)
static inline void gf_trace_int(uint64_t tag, const char* name, int32_t value)
{
    if (CC_UNLIKELY(gf_trace_is_tag_enabled(tag))) {
        void gf_trace_int_body(const char*, int32_t);
        gf_trace_int_body(name, value);
    }
}

/**
 * Traces a 64-bit integer counter value.  name is used to identify the
 * counter. This can be used to track how a value changes over time.
 */
#define GF_TRACE_INT64(name, value) gf_trace_int64(GF_TRACE_TAG, name, value)
static inline void gf_trace_int64(uint64_t tag, const char* name, int64_t value)
{
    if (CC_UNLIKELY(gf_trace_is_tag_enabled(tag))) {
        void gf_trace_int64_body(const char*, int64_t);
        gf_trace_int64_body(name, value);
    }
}

__END_DECLS

// enable GF_TRACE_NAME and GF_TRACE_CALL for C++
#ifdef __cplusplus

class ScopedTrace {
public:
inline ScopedTrace(uint64_t tag, const char* name)
    : mTag(tag) {
    gf_trace_begin(mTag,name);
}

inline ~ScopedTrace() {
    gf_trace_end(mTag);
}

private:
    uint64_t mTag;
};

// GF_TRACE_NAME traces the beginning and end of the current scope.  To trace
// the correct start and end times this macro should be declared first in the
// scope body.
#define GF_TRACE_NAME(name) ScopedTrace ___tracer(GF_TRACE_TAG, name)
// GF_TRACE_CALL is an GF_TRACE_NAME that uses the current function name.
#define GF_TRACE_CALL() GF_TRACE_NAME(__FUNCTION__)

#else // __cplusplus
#define GF_TRACE_NAME(...)
#define GF_TRACE_CALL()

#endif // __cplusplus

#else // GF_USERMODE_TRACE

#define GF_TRACE_INIT()
#define GF_TRACE_GET_ENABLED_TAGS()
#define GF_TRACE_ENABLED() 0
#define GF_TRACE_BEGIN(name, ...)
#define GF_TRACE_ARGS(format, ...)
#define GF_TRACE_END()
#define GF_TRACE_END2(format, ...)
#define GF_TRACE_ASYNC_BEGIN(name, cookie)
#define GF_TRACE_ASYNC_END(name, cookie)
#define GF_TRACE_INT(name, value)
#define GF_TRACE_NAME(...)
#define GF_TRACE_CALL()

#endif // GF_USERMODE_TRACE

#endif // __GF_TRACE_H__
