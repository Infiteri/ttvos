#include "hal/hal.h"
#include "memory.h"
#include "stdio.h"
#include <stdint.h>

extern uint8_t __bss_start;
extern uint8_t __end;

void __attribute__((section(".entry"))) start(uint16_t bootDrive)
{
    memset(&__bss_start, 0, (&__end) - (&__bss_start));

    HALInitialize();

    clrscr();

    printf("Hello world from kernel!!!\n");

end:
    for (;;)
        ;
}
