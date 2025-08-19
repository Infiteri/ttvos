#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef void *ptr_t;

typedef enum
{
    REGION_TYPE_FREE,
    REGION_TYPE_RESERVED,
    REGION_TYPE_UNMAPPED,
    REGION_TYPE_ALLOCATOR,
} RegionType;

typedef struct
{
    ptr_t Base;
    uint64_t Size;
    RegionType Type;
} AllocatorMemoryRegion;

/// @brief MemoryRegion but 'Base' and 'Size' are in blocks
typedef struct
{
    uint64_t Base;
    uint64_t Size;
    RegionType Type;
} MemoryRegionBlocks;

typedef struct
{
    uint64_t MemSize;
    uint64_t MemSizeBytes;
    uint64_t MemBlockSize;
    uint8_t *MemBase;
} AllocatorBase;

// These are basic functions all allocator types will need. All alocators will
// have an AllocatorBase in their respective structures. Implementation of
// configurator functions will be added. They will provide 'constructors' for
// the AllocatorBase inside each allocator type

uint64_t Allocator_ToBlock(void *ptr, AllocatorBase *base);
uint64_t Allocator_ToBlockRoundUp(void *ptr, AllocatorBase *base);
ptr_t *Allocator_ToPtr(uint64_t block, AllocatorBase *base);
bool Allcator_InitializeBase(AllocatorBase *base, uint64_t blockSize,
                             const AllocatorMemoryRegion *regions,
                             uint64_t regionCount);
void Allocator_DetermineMemoryRange(AllocatorBase *base,
                                    const AllocatorMemoryRegion *regions,
                                    uint64_t regionCount);

void Allocator_FixOverlappingRegions(AllocatorBase *base,
                                     MemoryRegionBlocks *blocks,
                                     uint64_t *regionCount);
