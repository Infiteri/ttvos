#include "arch/i686/irq.h"
#include "arch/i686/isr.h"
#include "debug/debug.h"
#include "hal/hal.h"
#include "memory.h"
#include "memory/allocator.h"
#include "stdio.h"
#include <boot/bootparams.h>
#include <stdint.h>

extern uint8_t __bss_start;
extern uint8_t __end;

void timer(Registers *reg) { printf("."); }
void timer1(Registers *reg) { printf("1"); }

static inline RegionType MapE820Type(uint32_t t)
{
    switch (t)
    {
    case 1:
        return REGION_TYPE_FREE;
    case 2:
        return REGION_TYPE_RESERVED;
    case 3:
        return REGION_TYPE_RESERVED; // ACPI reclaimable (don’t use yet)
    case 4:
        return REGION_TYPE_RESERVED; // ACPI NVS
    case 5:
        return REGION_TYPE_RESERVED; // bad memory
    default:
        return REGION_TYPE_RESERVED; // 13 and anything else
    }
}

static void DetermineAllocatorRange(const BootParams *bp, uint64_t *outPhysBase,
                                    uint64_t *outPhysEnd)
{
    uint64_t minB = ~0ULL, maxE = 0;
    for (int i = 0; i < bp->Memory.RegionCount; i++)
    {
        uint64_t b = bp->Memory.Regions[i].Begin;
        uint64_t l = bp->Memory.Regions[i].Length;
        if (l == 0)
            continue;

        if (b < minB)
            minB = b;
        if (b + l > maxE)
            maxE = b + l;
    }
    if (minB == ~0ULL)
        minB = 0;
    *outPhysBase = minB;
    *outPhysEnd = maxE;
}

void BuildAllocatorRegions(const BootParams *bootParams,
                           AllocatorMemoryRegion *outAllocatorRegions,
                           MemoryRegionBlocks *outBlockRegions,
                           uint64_t *outCount, AllocatorBase *allocatorBase)
{
    const uint64_t count = (uint64_t)bootParams->Memory.RegionCount;

    // 1) Choose block size and set allocator global range FIRST
    if (allocatorBase->MemBlockSize == 0)
        allocatorBase->MemBlockSize = 4096; // safety

    uint64_t physBase, physEnd;
    DetermineAllocatorRange(bootParams, &physBase, &physEnd);

    allocatorBase->MemBase = (uint8_t *)(uintptr_t)physBase;
    allocatorBase->MemSizeBytes =
        (physEnd > physBase) ? (physEnd - physBase) : 0;
    allocatorBase->MemSize =
        allocatorBase->MemSizeBytes / allocatorBase->MemBlockSize;

    // 2) Convert each BIOS region to allocator + block regions
    for (uint64_t i = 0; i < count; i++)
    {
        uint64_t begin = bootParams->Memory.Regions[i].Begin;
        uint64_t length = bootParams->Memory.Regions[i].Length;
        uint32_t type = bootParams->Memory.Regions[i].Type;

        RegionType mapped = MapE820Type(type);

        // Byte-based (keep as physical; avoids truncation in 32-bit)
        outAllocatorRegions[i].Base = (void *)(uintptr_t)begin;
        outAllocatorRegions[i].Size = length;
        outAllocatorRegions[i].Type = mapped;

        // Block-based (relative to MemBase)
        uint64_t relStart = (begin >= physBase) ? (begin - physBase) : 0;
        uint64_t startBlk = relStart / allocatorBase->MemBlockSize;
        uint64_t sizeBlk = (length + allocatorBase->MemBlockSize - 1) /
                           allocatorBase->MemBlockSize; // ceil

        outBlockRegions[i].Base = startBlk;
        outBlockRegions[i].Size = sizeBlk;
        outBlockRegions[i].Type = mapped;
    }

    *outCount = count;
}

void __attribute__((section(".entry"))) start(BootParams *bootParams)
{
    memset(&__bss_start, 0, (&__end) - (&__bss_start));

    HAL_Initialize();
    clrscr();

    // Debug BIOS memory info
    DebugDebug("Boot device: %x\n", bootParams->BootDevice);
    DebugDebug("Memory region count: %d\n", bootParams->Memory.RegionCount);
    for (int i = 0; i < bootParams->Memory.RegionCount; i++)
    {
        DebugDebug("MEM: start=0x%llx length=0x%llx type=%i\n",
                   bootParams->Memory.Regions[i].Begin,
                   bootParams->Memory.Regions[i].Length,
                   bootParams->Memory.Regions[i].Type);
    }

    // --- Initialize allocator base ---
    AllocatorBase allocatorBase;
    allocatorBase.MemBlockSize = 4096; // 4 KiB
    allocatorBase.MemBase = 0;         // Will be set to lowest usable memory
    allocatorBase.MemSize = 0;
    allocatorBase.MemSizeBytes = 0;

    // Determine memory range from boot params
    uint64_t physBase = ~0ULL;
    uint64_t physEnd = 0;
    for (int i = 0; i < bootParams->Memory.RegionCount; i++)
    {
        uint64_t b = bootParams->Memory.Regions[i].Begin;
        uint64_t l = bootParams->Memory.Regions[i].Length;
        if (l == 0)
            continue;
        if (b < physBase)
            physBase = b;
        if (b + l > physEnd)
            physEnd = b + l;
    }

    allocatorBase.MemBase = (uint8_t *)(uintptr_t)physBase;
    allocatorBase.MemSizeBytes = physEnd - physBase;
    allocatorBase.MemSize =
        allocatorBase.MemSizeBytes / allocatorBase.MemBlockSize;

    // --- Build allocator regions ---
    AllocatorMemoryRegion allocatorRegions[32]; // adjust if needed
    MemoryRegionBlocks blockRegions[32];
    uint64_t regionCount = 0;

    BuildAllocatorRegions(bootParams, allocatorRegions, blockRegions,
                          &regionCount, &allocatorBase);

    // --- Debug allocator regions ---
    DebugDebug("\nAllocator regions:\n");
    for (uint64_t i = 0; i < regionCount; i++)
    {
        DebugDebug("[%llu] Base=%p Size=0x%llx Type=%d\n", i,
                   allocatorRegions[i].Base, allocatorRegions[i].Size,
                   allocatorRegions[i].Type);
    }

    // --- Debug block regions ---
    DebugDebug("\nBlock regions:\n");
    for (uint64_t i = 0; i < regionCount; i++)
    {
        DebugDebug("[%llu] Base=%llu Size=%llu Type=%d\n", i,
                   blockRegions[i].Base, blockRegions[i].Size,
                   blockRegions[i].Type);
    }

    // --- Register IRQs ---
    i686_IRQ_RegisterHanlder(0, timer);
    i686_IRQ_RegisterHanlder(1, timer1);

end:
    for (;;)
        ;
}
