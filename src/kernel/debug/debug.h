#pragma once

typedef enum
{
    LVL_DEBUG = 0,
    LVL_INFO = 1,
    LVL_WARN = 2,
    LVL_ERROR = 3,
} LogLevel;

void DebugPutc(char c);
void DebugPuts(const char *str);
void DebugPrintf(LogLevel level, const char *fmt, ...);

#define DebugDebug(fmt, ...) DebugPrintf(LVL_DEBUG, fmt, ##__VA_ARGS__)
#define DebugInfo(fmt, ...) DebugPrintf(LVL_INFO, fmt, ##__VA_ARGS__)
#define DebugWarn(fmt, ...) DebugPrintf(LVL_WARN, fmt, ##__VA_ARGS__)
#define DebugError(fmt, ...) DebugPrintf(LVL_ERROR, fmt, ##__VA_ARGS__)
