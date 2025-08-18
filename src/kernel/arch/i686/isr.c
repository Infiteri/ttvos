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
}

void __attribute__((cdecl)) i686_ISR_Handler(Registers *regs)
{

    if (g_Handlers[regs->Interrupt] != NULL)
        g_Handlers[regs->Interrupt](regs);
    else if (regs->Interrupt >= 32)
        printf("Unhandled interrupt %i\n", regs->Interrupt);
    else
    {
        printf("Unhandled exception %d %s\n", regs->Interrupt,
               g_Exceptions[regs->Interrupt]);

        printf("  eax=%x  ebx=%x  ecx=%x  edx=%x  esi=%x  edi=%x\n", regs->eax,
               regs->ebx, regs->ecx, regs->edx, regs->esi, regs->edi);

        printf("  esp=%x  ebp=%x  eip=%x  eflags=%x  cs=%x  ds=%x  ss=%x\n",
               regs->esp, regs->ebp, regs->eip, regs->eflags, regs->cs,
               regs->ds, regs->ss);

        printf("  interrupt=%x  errorcode=%x\n", regs->Interrupt, regs->Error);

        printf("KERNEL PANIC!\n");
        i686_panic();
    }
}

void i686_ISR_RegisterHandler(int interrupt, ISRHandler handler)
{
    g_Handlers[interrupt] = handler;
    i686_IDTEnableGate(interrupt);
}
