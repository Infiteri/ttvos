#include "string.h"
#include "stdint.h"

const char *StrChr(const char *str, char c)
{
    if (str == NULL)
        return NULL;

    while (*str)
    {
        if (*str == c)
            return str;

        ++str;
    }

    return NULL;
}

char *StrCpy(char *dst, const char *src)
{
    char *orig = dst;
    if (dst == NULL)
        return NULL;

    if (src == NULL)
    {
        *dst = '\0';
        return dst;
    }

    while (*src)
    {
        *dst = *src;
        ++src;
        ++dst;
    }

    *dst = '\0';
    return orig;
}

unsigned StrLen(const char *src)
{
    unsigned len = 0;
    while (*src)
    {
        len++;
        src++;
    }

    return len;
}
