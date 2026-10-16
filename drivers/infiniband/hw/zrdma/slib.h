#ifndef __SLIB_H__
#define __SLIB_H__
#ifdef __cplusplus
extern "C" {
#endif

#include <linux/stddef.h>
#include <linux/string.h>

void *zrdma_memcpy_s(void *dest, const void *src, size_t n);
void *zrdma_memset_s(void *s, int c, size_t n);
size_t zrdma_strlen_s(const char *s);
int zrdma_snprintf_s(char *buf, size_t size, const char *format, ...) __attribute__((format(gnu_printf, 3, 4)));
int zrdma_sscanf_s(const char *buf, const char *format, ...) __attribute__((format(gnu_scanf, 2, 3)));

#ifdef __cplusplus
}
#endif

#endif