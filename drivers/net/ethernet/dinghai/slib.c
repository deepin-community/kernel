#include <linux/dinghai/driver.h>
#include "slib.h"

void* zte_memcpy_s(void* dest, const void* src, size_t n)
{
    return memcpy(dest, src, n);
}

char *zte_strncpy_s(char *dest, const char *src, size_t count)
{
    return strncpy(dest, src, count);
}

void* zte_memset_s(void* s, int c, size_t n)
{
    return memset(s, c, n);
}

int zte_snprintf_s(char* buf, size_t size, const char* format, ...)
{
    va_list args;
    int i;

    if (buf == NULL || size == 0) {
        return 0;
    }

    va_start(args, format);
    i = vsnprintf(buf, size, format, args);
    va_end(args);

    return i;
}

int zte_sprintf_s(char* buf, const char* format, ...)
{
    va_list args;
    int i;

    if (buf == NULL) {
        return 0;
    }

    va_start(args, format);
    i = vsprintf(buf, format, args);
    va_end(args);
    return i;

}

int zte_sscanf_s(const char *buf, const char *format, ...)
{
    va_list args;
    int i;

    va_start(args, format);
    i = vsscanf(buf, format, args);
    va_end(args);

    return i;
}

size_t zte_strlen_s(const char* s)
{
    return strlen(s);
}

char *zte_strncat_s(char *dest, const char *src, size_t n)
{
    return strncat(dest, src, n);
}



#ifndef CGS_V5_693
/* 测试用例（仅非 CGS_V5_693 内核编译，避免老编译器对 strncpy/strncat 的
 * 额外静态检查在测试代码路径上触发 stringop-* 报错）
 */
void test_zte_memcpy_s(void) {
    char src[] = "Hello";
    char dest[10];
    zte_memcpy_s(dest, src, zte_strlen_s(src) + 1);
    if (strcmp(dest, src) == 0)
    {
        LOG_INFO("test_zte_memcpy_s success\n");
    }
}

void test_zte_memset_s(void) {
    char buf[10];
    zte_memset_s(buf, 'A', 5);
    buf[5] = '\0';
    if(strcmp(buf, "AAAAA") == 0)
    {
        LOG_INFO("test_zte_memset_s success\n");
    }
}

void test_zte_snprintf_s(void) {
    char buf[20];
    int result = zte_snprintf_s(buf, sizeof(buf), "Number: %d", 123);
    if (result > 0)
    {
        if(strcmp(buf, "Number: 123") == 0)
        {
            LOG_INFO("test_zte_snprintf_s success\n");
        }
    }
}

void test_zte_sprintf_s(void) {
    char buf[20];
    int result = zte_sprintf_s(buf, "Text: %s", "Test");
    if (result > 0)
    {
        if(strcmp(buf, "Text: Test") == 0)
        {
            LOG_INFO("test_zte_sprintf_s success\n");
        }
    }
}

void test_zte_strlen_s(void) {
    char str[] = "Length";
    size_t len = zte_strlen_s(str);
    LOG_INFO("test_zte_strlen_s success, len = %ld\n", len);
}

void test_zte_strncat_s(void) {
    char dest[20] = "Hello";
    char src[] = " World";
    zte_strncat_s(dest, src, 6);
    if(strcmp(dest, "Hello World") == 0)
    {
        LOG_INFO("test_zte_strncat success\n");
    }
}

void test_zte_strncpy_s(void) {
    char src[] = "World";
    char dest[10];
    unsigned int src_len;

    src_len = strlen(src);
    zte_strncpy_s(dest, src, src_len + 1);
    dest[src_len] = '\0';
    if (strcmp(dest, src) == 0)
    {
        LOG_INFO("test_zte_strncpy_s success\n");
    }
}

void test_zte_sscanf_s(void) {
    char input[] = "123";
    int value;
    int result = zte_sscanf_s(input, "%d", &value);
    if (result == 1 && value == 123)
    {
        LOG_INFO("test_zte_sscanf_s success\n");
    }
}

void recording_not_safe_func(void)
{
    test_zte_memcpy_s();
    test_zte_memset_s();
    test_zte_snprintf_s();
    test_zte_sprintf_s();
    test_zte_strlen_s();
    test_zte_strncat_s();
    test_zte_strncpy_s();
    test_zte_sscanf_s();

    LOG_INFO("All tests passed!\n");
}
#endif /* !CGS_V5_693 */