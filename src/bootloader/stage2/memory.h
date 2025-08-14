#pragma once

#include "stdint.h"

#define MEMORY_MIN 0x00000500
#define MEMORY_MAX 0x00080000

#define MEMORY_FAT_ADDR (void far *)(0x00500000)
#define MEMORY_FAT_SIZE 0x00010000

void far *MemCpy(void far *dst, const void far *src, uint16_t num);
void far *MemSet(void far *ptr, int value, uint16_t num);
int MemCmp(const void far *ptr1, const void far *ptr2, uint16_t num);
