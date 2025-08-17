#include "isr.h"
#include "gdt.h"
#include "idt.h"
#include "io.h"
#include <stdio.h>

ISRHandler g_Handlers[256];

static const char *const g_Exceptions[] = {"Divide by zero error",
                                           "Debug",
                                           "Non-maskable Interrupt",
                                           "Breakpoint",
                                           "Overflow",
                                           "Bound Range Exceeded",
                                           "Invalid Opcode",
                                           "Device Not Available",
                                           "Double Fault",
                                           "Coprocessor Segment Overrun",
                                           "Invalid TSS",
                                           "Segment Not Present",
                                           "Stack-Segment Fault",
                                           "General Protection Fault",
                                           "Page Fault",
                                           "",
                                           "x87 Floating-Point Exception",
                                           "Alignment Check",
                                           "Machine Check",
                                           "SIMD Floating-Point Exception",
                                           "Virtualization Exception",
                                           "Control Protection Exception ",
                                           "",
                                           "",
                                           "",
                                           "",
                                           "",
                                           "",
                                           "Hypervisor Injection Exception",
                                           "VMM Communication Exception",
                                           "Security Exception",
                                           ""};

void i686_ISR_InitializeGates();

void i686_ISRInitialize()
{
    i686_ISR_InitializeGates();
    for (int i = 0; i < 256; i++)
        i686_IDTEnableGate(i);

    i686_IDTDisableGate(50);
}

void __attribute__((cdecl)) i686_ISR_Handler(Registers *reg)
{

    if (g_Handlers[reg->Interrupt] != NULL)
        g_Handlers[reg->Interrupt](reg);
    else if (reg->Interrupt >= 32)
        printf("Unhandled interrupt %i\n", reg->Interrupt);
    else
    {
        printf("Unhandled exception %i '%s'\n", reg->Interrupt,
               g_Exceptions[reg->Interrupt]);
        printf("KERNEL PANIC \n");
        i686_panic();
    }
}

void i686_ISRRegisterHanlder(int interrupt) {}
