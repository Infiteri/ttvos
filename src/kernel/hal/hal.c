#include "hal.h"

#include "arch/i686/gdt.h"
#include "arch/i686/idt.h"

void HALInitialize()
{
    i686_GDTInitialize();
    i686_IDTInitialize();
}
