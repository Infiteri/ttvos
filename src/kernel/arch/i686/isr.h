#pragma once

#include <stdint.h>
typedef struct
{
    uint32_t es;
    uint32_t edi, esi, ebp, kern_esp, ebx, edx, ecx, eax;
    uint32_t Interrupt, Error;
    uint32_t eip, cs, eflags, esp, ss;
} __attribute__((packed)) Registers;

typedef void (*ISRHandler)(Registers *regs);

void i686_ISRInitialize();
void i686_ISRRegisterHanlder(int interrupt);
