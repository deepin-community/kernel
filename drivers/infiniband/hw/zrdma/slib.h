#ifndef __SLIB_H__
#define __SLIB_H__
#ifdef __cplusplus
extern "C" {
#endif

#include <linux/stddef.h>
#include <linux/string.h>

void* zte_memcpy_s(void* dest, const void* src, size_t n);
void* zte_memset_s(void* s, int c, size_t n);
int zte_snprintf_s(char* buf, size_t size, const char* format, ...) __attribute__((format(gnu_printf, 3, 4)));
int zte_sprintf_s(char* buf, const char* format, ...)__attribute__((format(gnu_printf, 2, 3)));
size_t zte_strlen_s(const char* s);
char *zte_strncat_s(char *dest, const char *src, size_t n);
char *zte_strncpy_s(char *dest, const char *src, size_t count);
int zte_sscanf_s(const char *buf, const char *format, ...) __attribute__((format(gnu_scanf, 2, 3)));
void recording_not_safe_func(void);

/* 测试用例 */
void test_zte_memcpy_s(void);
void test_zte_memset_s(void);
void test_zte_snprintf_s(void);
void test_zte_sprintf_s(void);
void test_zte_strlen_s(void);
void test_zte_strncat_s(void);
void test_zte_strncpy_s(void);
void test_zte_sscanf_s(void);
void recording_not_safe_func(void);

#ifdef __cplusplus
}
#endif

#endif
