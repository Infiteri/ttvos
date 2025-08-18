#include "arch/i686/irq.h"
#include "arch/i686/isr.h"
#include "debug/debug.h"
#include "hal/hal.h"
#include "memory.h"
#include "stdio.h"
#include <boot/bootparams.h>
#include <stdint.h>

extern uint8_t __bss_start;
extern uint8_t __end;

void timer(Registers *reg) { printf("."); }
void timer1(Registers *reg) { printf("1"); }

void __attribute__((section(".entry"))) start(BootParams *bootParams)
{
    memset(&__bss_start, 0, (&__end) - (&__bss_start));

    HAL_Initialize();

    clrscr();

    // note that this goes in the terminal of the qemu host
    //
    DebugDebug("Boot device: %x\n", bootParams->BootDevice);
    DebugDebug("Memory region count: %d\n", bootParams->Memory.RegionCount);
    for (int i = 0; i < bootParams->Memory.RegionCount; i++)
    {
        DebugDebug("MEM: start=0x%llx length=0x%llx type=%x\n",
                   bootParams->Memory.Regions[i].Begin,
                   bootParams->Memory.Regions[i].Length,
                   bootParams->Memory.Regions[i].Type);
    }

    i686_IRQ_RegisterHanlder(0, timer);
    i686_IRQ_RegisterHanlder(1, timer1);

end:
    for (;;)
        ;
}
