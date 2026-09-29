#ifndef __SLIB_H__
#define __SLIB_H__
#ifdef __cplusplus
extern "C" {
#endif

#include <linux/stddef.h>
#include <linux/string.h>
#include <linux/dinghai/kcompat.h>

void* zte_memcpy_s(void* dest, const void* src, size_t n);
void* zte_memset_s(void* s, int c, size_t n);
int zte_snprintf_s(char* buf, size_t size, const char* format, ...);
int zte_sprintf_s(char* buf, const char* format, ...);
size_t zte_strlen_s(const char* s);
char *zte_strncat_s(char *dest, const char *src, size_t n);
char *zte_strncpy_s(char *dest, const char *src, size_t count);
int zte_sscanf_s(const char *buf, const char *format, ...);
void recording_not_safe_func(void);

#ifdef __cplusplus
}
#endif

#endif
