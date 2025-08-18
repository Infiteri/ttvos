#include "hal.h"

#include "arch/i686/gdt.h"
#include "arch/i686/idt.h"
#include "arch/i686/irq.h"
#include "arch/i686/isr.h"

void HALInitialize()
{
    i686_GDTInitialize();
    i686_IDTInitialize();
    i686_ISRInitialize();
    i686_IRQ_Initialize();
}
