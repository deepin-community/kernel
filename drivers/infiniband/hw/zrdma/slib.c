#include <linux/kernel.h>
#include "slib.h"

void *zrdma_memcpy_s(void *dest, const void *src, size_t n)
{
	return memcpy(dest, src, n);
}

void *zrdma_memset_s(void *s, int c, size_t n)
{
	return memset(s, c, n);
}

size_t zrdma_strlen_s(const char *s)
{
	return strlen(s);
}

int zrdma_snprintf_s(char *buf, size_t size, const char *format, ...)
{
	va_list args;
	int i;

	if (buf == NULL || size == 0)
		return 0;

	va_start(args, format);
	i = vsnprintf(buf, size, format, args);
	va_end(args);

	return i;
}

int zrdma_sscanf_s(const char *buf, const char *format, ...)
{
	va_list args;
	int i;

	va_start(args, format);
	i = vsscanf(buf, format, args);
	va_end(args);

	return i;
}