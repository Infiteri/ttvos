#pragma once

#include "isr.h"

typedef void (*IRQHandler)(Registers *reg);

void i686_IRQ_Initialize();
void i686_IRQ_RegisterHanlder(int irq, IRQHandler handler);
