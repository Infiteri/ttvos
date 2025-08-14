#pragma once

#include "disk.h"
#include "stdint.h"

#pragma pack(push, 1)

typedef struct
{
    uint8_t Name[11];
    uint8_t Attributes;
    uint8_t _Reserved;
    uint8_t CreatedTimeTenths;
    uint16_t CreatedTime;
    uint16_t CreatedDate;
    uint16_t AccessedDate;
    uint16_t FirstClusterHigh;
    uint16_t ModifiedTime;
    uint16_t ModifiedDate;
    uint16_t FirstClusterLow;
    uint32_t Size;
} FatDirectoryEntry;

#pragma pack(pop)

typedef struct
{
    int Handle;
    bool IsDirectory;
    uint32_t Position;
    uint32_t Size;
} FatFile;

enum FatAttributes
{
    FAT_ATTRIBUTE_READ_ONLY = 0x01,
    FAT_ATTRIBUTE_HIDDEN = 0x02,
    FAT_ATTRIBUTE_SYSTEM = 0x04,
    FAT_ATTRIBUTE_VOLUME_ID = 0x08,
    FAT_ATTRIBUTE_DIRECTORY = 0x10,
    FAT_ATTRIBUTE_ARCHIVE = 0x20,
    FAT_ATTRIBUTE_LFN = FAT_ATTRIBUTE_READ_ONLY | FAT_ATTRIBUTE_HIDDEN | FAT_ATTRIBUTE_SYSTEM |
                        FAT_ATTRIBUTE_VOLUME_ID,
};

bool FatInitialize(Disk *disk);
FatFile far *FatOpen(Disk *disk, const char *path);
uint32_t FatRead(Disk *disk, FatFile far *file, uint32_t byteCount, void *out);
bool FatReadEntry(Disk *disk, FatFile far *file, FatDirectoryEntry *dir);
void FatClose(FatFile far *file);
