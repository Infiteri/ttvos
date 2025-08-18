#include "memdetect.h"
#include "stdio.h"
#include "x86.h"

#define MAX_MEMORY_REGIONS 256

MemoryRegion g_MemRegions[MAX_MEMORY_REGIONS];
int g_MemoryRegionCount;

void Memory_Detect(MemoryInfo *info)
{
    E820MemoryBlock block;
    uint32_t continuation = 0;
    int ret;

    g_MemoryRegionCount = 0;

    ret = x86_E820GetNextBlock(&block, &continuation);

    while (ret > 0 && continuation != 0)
    {
        g_MemRegions[g_MemoryRegionCount].Begin = block.Base;
        g_MemRegions[g_MemoryRegionCount].Length = block.Length;
        g_MemRegions[g_MemoryRegionCount].Type = block.Type;
        g_MemRegions[g_MemoryRegionCount].ACPI = block.ACPI;
        g_MemoryRegionCount++;

        printf("E820: base=0x%llx length=0x%llx type=0x%x \n", block.Base,
               block.Length, block.Type);

        ret = x86_E820GetNextBlock(&block, &continuation);
    };

    info->RegionCount = g_MemoryRegionCount;
    info->Regions = g_MemRegions;
}
