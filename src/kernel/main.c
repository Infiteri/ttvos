#include "arch/i686/irq.h"
#include "arch/i686/isr.h"
#include "debug/debug.h"
#include "hal/hal.h"
#include "memory.h"
#include "stdio.h"
#include <stdint.h>

extern uint8_t __bss_start;
extern uint8_t __end;

void timer(Registers *reg) { printf("."); }

void __attribute__((section(".entry"))) start(uint16_t bootDrive)
{
    memset(&__bss_start, 0, (&__end) - (&__bss_start));

    HALInitialize();

    clrscr();

    // note that this goes in the terminal of the qemu host
    DebugInfo("Hello world %s\n", "step");
    DebugDebug("Hello world %s\n", "step");
    DebugWarn("Hello world %s\n", "step");
    DebugError("Hello world %s\n", "step");

end:
    for (;;)
        ;
}
