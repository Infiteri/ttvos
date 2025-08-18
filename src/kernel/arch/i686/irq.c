#include "irq.h"
#include "debug/debug.h"
#include "io.h"
#include "isr.h"
#include "pic.h"
#include <stdio.h>

#define PIC_REMAP_OFFSET 0x20

IRQHandler g_IRQHandlers[16];

void i686_IRQ_Handler(Registers *reg)
{
    int irq = reg->Interrupt - PIC_REMAP_OFFSET;

    uint8_t pic_isr = i686_PIC_ReadInServiceRegister();
    uint8_t pic_irr = i686_PIC_ReadIRQRequestRegister();

    if (g_IRQHandlers[irq] != NULL)
    {
        // handle IRQ
        g_IRQHandlers[irq](reg);
    }
    else
    {
        printf("Unhandled IRQ %d  ISR=%x  IRR=%x...\n", irq, pic_isr, pic_irr);
    }

    // send EOI
    i686_PIC_SendEndOfInterrupt(irq);
}

void i686_IRQ_Initialize()
{
    i686_PIC_Configure(PIC_REMAP_OFFSET, PIC_REMAP_OFFSET + 8);

    DebugDebug("Init irq %i\n");
    for (int i = 0; i < 16; i++)
        i686_ISR_RegisterHandler(PIC_REMAP_OFFSET + i, i686_IRQ_Handler);

    i686_EnableInterrupts();
}

void i686_IRQ_RegisterHanlder(int irq, IRQHandler handler)
{
    DebugDebug("Added irq handler %i\n", irq);
    g_IRQHandlers[irq] = handler;
}
