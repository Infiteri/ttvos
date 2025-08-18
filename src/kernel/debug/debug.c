#include "debug.h"
#include "arch/i686/io.h"
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>

void DebugPutc(char c) { i686_outb(0xE9, c); }

void DebugPuts(const char *str)
{
    while (*str)
    {
        DebugPutc(*str);
        str++;
    }
}

static const char g_HexChars[] = "0123456789abcdef";

static void debug_printf_unsigned(unsigned long long number, int radix)
{
    char buffer[32];
    int pos = 0;

    // convert number to ASCII
    do
    {
        unsigned long long rem = number % radix;
        number /= radix;
        buffer[pos++] = g_HexChars[rem];
    } while (number > 0);

    // print number in reverse order
    while (--pos >= 0)
        DebugPutc(buffer[pos]);
}

static void debug_printf_signed(long long number, int radix)
{
    if (number < 0)
    {
        DebugPutc('-');
        debug_printf_unsigned(-number, radix);
    }
    else
        debug_printf_unsigned(number, radix);
}

#define PRINTF_STATE_NORMAL 0
#define PRINTF_STATE_LENGTH 1
#define PRINTF_STATE_LENGTH_SHORT 2
#define PRINTF_STATE_LENGTH_LONG 3
#define PRINTF_STATE_SPEC 4

#define PRINTF_LENGTH_DEFAULT 0
#define PRINTF_LENGTH_SHORT_SHORT 1
#define PRINTF_LENGTH_SHORT 2
#define PRINTF_LENGTH_LONG 3
#define PRINTF_LENGTH_LONG_LONG 4

static const char *const g_LogSeverityColors[] = {
    [LVL_DEBUG] = "\033[2;37m",
    [LVL_INFO] = "\033[37m",
    [LVL_WARN] = "\033[1;33m",
    [LVL_ERROR] = "\033[1;31m",
};

void DebugPrintf(LogLevel level, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    int state = PRINTF_STATE_NORMAL;
    int length = PRINTF_LENGTH_DEFAULT;
    int radix = 10;
    bool sign = false;
    bool number = false;

    DebugPuts(g_LogSeverityColors[level]);

    while (*fmt)
    {
        switch (state)
        {
        case PRINTF_STATE_NORMAL:
            switch (*fmt)
            {
            case '%':
                state = PRINTF_STATE_LENGTH;
                break;
            default:
                DebugPutc(*fmt);
                break;
            }
            break;

        case PRINTF_STATE_LENGTH:
            switch (*fmt)
            {
            case 'h':
                length = PRINTF_LENGTH_SHORT;
                state = PRINTF_STATE_LENGTH_SHORT;
                break;
            case 'l':
                length = PRINTF_LENGTH_LONG;
                state = PRINTF_STATE_LENGTH_LONG;
                break;
            default:
                goto PRINTF_STATE_SPEC_;
            }
            break;

        case PRINTF_STATE_LENGTH_SHORT:
            if (*fmt == 'h')
            {
                length = PRINTF_LENGTH_SHORT_SHORT;
                state = PRINTF_STATE_SPEC;
            }
            else
                goto PRINTF_STATE_SPEC_;
            break;

        case PRINTF_STATE_LENGTH_LONG:
            if (*fmt == 'l')
            {
                length = PRINTF_LENGTH_LONG_LONG;
                state = PRINTF_STATE_SPEC;
            }
            else
                goto PRINTF_STATE_SPEC_;
            break;

        case PRINTF_STATE_SPEC:
        PRINTF_STATE_SPEC_:
            switch (*fmt)
            {
            case 'c':
                DebugPutc((char)va_arg(args, int));
                break;

            case 's':
                DebugPuts(va_arg(args, const char *));
                break;

            case '%':
                DebugPutc('%');
                break;

            case 'd':
            case 'i':
                radix = 10;
                sign = true;
                number = true;
                break;

            case 'u':
                radix = 10;
                sign = false;
                number = true;
                break;

            case 'X':
            case 'x':
            case 'p':
                radix = 16;
                sign = false;
                number = true;
                break;

            case 'o':
                radix = 8;
                sign = false;
                number = true;
                break;

            // ignore invalid spec
            default:
                break;
            }

            if (number)
            {
                if (sign)
                {
                    switch (length)
                    {
                    case PRINTF_LENGTH_SHORT_SHORT:
                    case PRINTF_LENGTH_SHORT:
                    case PRINTF_LENGTH_DEFAULT:
                        debug_printf_signed(va_arg(args, int), radix);
                        break;

                    case PRINTF_LENGTH_LONG:
                        debug_printf_signed(va_arg(args, long), radix);
                        break;

                    case PRINTF_LENGTH_LONG_LONG:
                        debug_printf_signed(va_arg(args, long long), radix);
                        break;
                    }
                }
                else
                {
                    switch (length)
                    {
                    case PRINTF_LENGTH_SHORT_SHORT:
                    case PRINTF_LENGTH_SHORT:
                    case PRINTF_LENGTH_DEFAULT:
                        debug_printf_unsigned(va_arg(args, unsigned int),
                                              radix);
                        break;

                    case PRINTF_LENGTH_LONG:
                        debug_printf_unsigned(va_arg(args, unsigned long),
                                              radix);
                        break;

                    case PRINTF_LENGTH_LONG_LONG:
                        debug_printf_unsigned(va_arg(args, unsigned long long),
                                              radix);
                        break;
                    }
                }
            }

            // reset state
            state = PRINTF_STATE_NORMAL;
            length = PRINTF_LENGTH_DEFAULT;
            radix = 10;
            sign = false;
            number = false;
            break;
        }

        fmt++;
    }

    va_end(args);
}

static void debug_print_buffer(const char *msg, const void *buffer,
                               uint32_t count)
{
    const uint8_t *u8Buffer = (const uint8_t *)buffer;

    DebugPuts(msg);
    for (uint16_t i = 0; i < count; i++)
    {
        DebugPutc(g_HexChars[u8Buffer[i] >> 4]);
        DebugPutc(g_HexChars[u8Buffer[i] & 0xF]);
    }
    DebugPuts("\n");
}
