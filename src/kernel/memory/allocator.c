#include "allocator.h"
#include "math/math.h"
#include "memory.h"
#include <stddef.h>
#include <stdint.h>

uint64_t Allocator_ToBlock(void *ptr, AllocatorBase *base)
{
    uint8_t *u8Ptr = (uint8_t *)(ptr);
    return (u8Ptr - base->MemBase) / base->MemBlockSize;
}

uint64_t Allocator_ToBlockRoundUp(void *ptr, AllocatorBase *base)
{

    uint8_t *u8Ptr = (uint8_t *)(ptr);
    return UINT64_DivRoundUp((uint64_t)(u8Ptr - base->MemBase),
                             base->MemBlockSize);
}

ptr_t *Allocator_ToPtr(uint64_t block, AllocatorBase *base)
{
    uint8_t *u8 = base->MemBase + block * base->MemBlockSize;
    return (ptr_t)(u8);
}

bool Allcator_InitializeBase(AllocatorBase *base, uint64_t blockSize,
                             const AllocatorMemoryRegion *regions,
                             uint64_t regionCount)
{
    base->MemBlockSize = blockSize;
    Allocator_DetermineMemoryRange(base, regions, regionCount);
    MemoryRegionBlocks tempRegions[1024];
    if (regionCount >= 1024)
        return false;

    for (size_t i = 0; i < regionCount; i++)
    {
        if (regions[i].Type == REGION_TYPE_FREE)
        {
            tempRegions[i].Base =
                Allocator_ToBlockRoundUp(base, regions[i].Base);
            tempRegions[i].Size = regions[i].Size / blockSize;
        }
        else
        {
            tempRegions[i].Base = Allocator_ToBlock(base, regions[i].Base);
            tempRegions[i].Size = UINT64_DivRoundUp(regions[i].Size, blockSize);
        }
        tempRegions[i].Type = regions[i].Type;
    }

    Allocator_FixOverlappingRegions(base, tempRegions, &regionCount);

    return true;
}

int RegionCompare(MemoryRegionBlocks *a, MemoryRegionBlocks *b)
{
    if (a->Base == b->Base)
        return (a->Size > b->Size) - (a->Size < b->Size);
    return (a->Base > b->Base) - (a->Base < b->Base);
}

static void Swap(MemoryRegionBlocks *a, MemoryRegionBlocks *b)
{
    MemoryRegionBlocks temp = *a;
    *a = *b;
    *b = temp;
}

static int Partition(MemoryRegionBlocks *arr, int low, int high)
{
    MemoryRegionBlocks *pivot = &arr[high];
    int i = low - 1;

    for (int j = low; j < high; j++)
    {
        if (RegionCompare(&arr[j], pivot) < 0)
        {
            i++;
            Swap(&arr[i], &arr[j]);
        }
    }
    Swap(&arr[i + 1], &arr[high]);
    return i + 1;
}

static void QuickSort(MemoryRegionBlocks *arr, int low, int high)
{
    if (low < high)
    {
        int pi = Partition(arr, low, high);
        QuickSort(arr, low, pi - 1);
        QuickSort(arr, pi + 1, high);
    }
}

void RegionsSort(MemoryRegionBlocks *blocks, uint64_t *size)
{
    if (*size > 1)
    {
        QuickSort(blocks, 0, (int)(*size - 1));
    }
}

void ArrayDeleteElement(void *array, uint64_t index, uint64_t *count)
{
    if (*count == 0)
        return;

    memmove(array + index, array + index + 1,
            sizeof(array[0]) * (*count - index - 1));
    (*count)--;
}

void Allocator_DetermineMemoryRange(AllocatorBase *base,
                                    const AllocatorMemoryRegion *regions,
                                    uint64_t regionCount)
{
    uint8_t *memBase = (uint8_t *)(-1);
    uint8_t *memEnd = NULL;

    for (uint64_t i = 0; i < regionCount; i++)
    {
        if (regions[i].Base < memBase)
            memBase = (uint8_t *)(regions[i].Base);

        if ((uint8_t *)(regions[i].Base) + regions[i].Size > memEnd)
            memEnd = (uint8_t *)(regions[i].Base) + regions[i].Size;
    }

    base->MemBase = memBase;
    base->MemSizeBytes = memEnd - memBase;
    base->MemSize = base->MemSizeBytes / base->MemBlockSize;
}

void Allocator_FixOverlappingRegions(AllocatorBase *base,
                                     MemoryRegionBlocks *blocks,
                                     uint64_t *regionCount)
{
    RegionsSort(blocks, regionCount);

    for (uint64_t i = 0; i < *regionCount; i++)
    {
        // Remove 0 sized regions
        if (blocks[i].Size == 0)
        {
            ArrayDeleteElement(blocks, i, regionCount);
            --i;
        }
        else if (i < *regionCount - 1)
        {
            // Regions of the same type overlapping/touching? Merge them
            if (blocks[i].Type == blocks[i + 1].Type &&
                blocks[i].Base + blocks[i].Size >= blocks[i + 1].Base)
            {
                uint64_t end = MAX(blocks[i].Base + blocks[i].Size,
                                   blocks[i + 1].Base + blocks[i + 1].Size);
                blocks[i].Size = end - blocks[i].Base;

                ArrayDeleteElement(blocks, i + 1, regionCount);
                --i;
            }
            else if (blocks[i].Type != blocks[i + 1].Type &&
                     blocks[i].Base + blocks[i].Size > blocks[i + 1].Base)
            {
                uint64_t overlapSize =
                    blocks[i].Base + blocks[i].Size - blocks[i + 1].Base;

                if (blocks[i].Type != REGION_TYPE_FREE)
                {
                    if (overlapSize < blocks[i + 1].Size)
                    {
                        blocks[i + 1].Base = blocks[i + 1].Base + overlapSize;
                        blocks[i + 1].Size -= overlapSize;
                    }
                    else
                    {
                        ArrayDeleteElement(blocks, i + 1, regionCount);
                        --i;
                    }
                }
                else if (overlapSize < blocks[i].Size)
                {
                    blocks[i].Size -= overlapSize;
                }
                else
                {
                    ArrayDeleteElement(blocks, i, regionCount);
                    --i;
                }
            }
        }
    }
}
