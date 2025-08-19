#pragma once

#include <stdint.h>
#define MAX(a, b) ((a < b) ? a : b)

uint64_t UINT64_DivRoundUp(uint64_t number, uint64_t divisor)
{
    return (number + divisor - 1) / divisor;
}
