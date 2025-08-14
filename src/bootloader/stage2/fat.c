#include "fat.h"
#include "ctype.h"
#include "disk.h"
#include "memory.h"
#include "stdint.h"
#include "stdio.h"
#include "string.h"
#include "utils.h"

#define SECTOR_SIZE 512
#define MAX_PATH_SIZE 256
#define MAX_FILE_HANDLES 10
#define ROOT_DIRECTORY_HANDLE -1

#pragma pack(push, 1)
typedef struct
{
    uint8_t BootJumpInstruction[3];
    uint8_t OemIdentifier[8];
    uint16_t BytesPerSector;
    uint8_t SectorsPerCluster;
    uint16_t ReservedSectors;
    uint8_t FatCount;
    uint16_t DirEntryCount;
    uint16_t TotalSectors;
    uint8_t MediaDescriptorType;
    uint16_t SectorsPerFat;
    uint16_t SectorsPerTrack;
    uint16_t Heads;
    uint32_t HiddenSectors;
    uint32_t LargeSectorCount;
    uint8_t DriveNumber;
    uint8_t _Reserved;
    uint8_t Signature;
    uint32_t VolumeId;       // serial number, value doesn't matter
    uint8_t VolumeLabel[11]; // 11 bytes, padded with spaces
    uint8_t SystemId[8];
} FatBootSector;

#pragma pack(pop)

typedef struct
{
    uint8_t Buffer[SECTOR_SIZE];
    FatFile Public;
    bool Opened;
    uint32_t FirstCluster;
    uint32_t CurrentCluster;
    uint32_t CurrentSectorInCluster;
} FatFileData;

typedef struct
{
    union
    {
        FatBootSector BootSector;
        uint8_t BootSectorBytes[SECTOR_SIZE];
    } BS;

    FatFileData RootDirectory;
    FatFileData OpenedFiles[MAX_FILE_HANDLES];
} FatData;

static FatData far *Data;
static uint8_t far *Fat = NULL;
static uint32_t DataSectionLba;

bool ReadBootSector(Disk *disk) { return DiskReadSectors(disk, 0, 1, Data->BS.BootSectorBytes); }

#define DS Data->BS.BootSector

uint32_t ClusterToLba(uint32_t cluster)
{
    return DataSectionLba + (cluster - 2) * DS.SectorsPerCluster;
}

bool ReadFat(Disk *disk)
{
    return DiskReadSectors(disk, Data->BS.BootSector.ReservedSectors,
                           Data->BS.BootSector.SectorsPerFat, Fat);
}

bool FatInitialize(Disk *disk)
{
    Data = (FatData far *)MEMORY_FAT_ADDR;

    if (!ReadBootSector(disk))
    {
        printf("Fat: Couldn't read boot sectors\r\n");
        return false;
    }

    Fat = (uint8_t far *)Data + sizeof(FatData);

    uint32_t size = Data->BS.BootSector.BytesPerSector * Data->BS.BootSector.SectorsPerFat;

    if (sizeof(FatData) + size >= MEMORY_FAT_SIZE)
    {
        printf("Fat: Not enough memory to read fat\r\n");
        return false;
    }

    if (!ReadFat(disk))
    {
        printf("Fat: Reading fat failed\r\n");
        return false;
    }

    uint32_t dirSize = sizeof(FatDirectoryEntry) * Data->BS.BootSector.DirEntryCount;
    uint32_t dirLba = DS.ReservedSectors + DS.SectorsPerFat * DS.FatCount;

    Data->RootDirectory.Opened = true;
    Data->RootDirectory.Public.Handle = ROOT_DIRECTORY_HANDLE;
    Data->RootDirectory.Public.IsDirectory = true;
    Data->RootDirectory.Public.Position = 0;
    Data->RootDirectory.Public.Size = sizeof(FatDirectoryEntry) + Data->BS.BootSector.DirEntryCount;
    Data->RootDirectory.FirstCluster = dirLba;
    Data->RootDirectory.CurrentCluster = dirLba;
    Data->RootDirectory.CurrentSectorInCluster = 0;

    if (!DiskReadSectors(disk, dirLba, 1, Data->RootDirectory.Buffer))
    {
        printf("Fat: Cannot read root directory\r\n");
        return false;
    }

    uint32_t rootDirSectors = (dirSize + DS.BytesPerSector - 1) / DS.BytesPerSector;
    DataSectionLba = dirLba + rootDirSectors;

    for (int i = 0; i < MAX_FILE_HANDLES; i++)
        Data->OpenedFiles[i].Opened = false;

    return true;
}

FatFile far *OpenDirEntry(Disk *disk, FatDirectoryEntry *entry)
{
    int handle = -1;
    for (int i = 0; i < MAX_FILE_HANDLES; i++)
    {
        if (!Data->OpenedFiles[i].Opened)
            handle = i;
    }

    if (handle < 0)
    {
        printf("Fat: Out of file handlers\r\n");
        return NULL;
    }

    FatFileData far *fd = &Data->OpenedFiles[handle];
    fd->Public.Handle = handle;
    fd->Public.IsDirectory = (entry->Attributes & FAT_ATTRIBUTE_DIRECTORY) != 0;
    fd->Public.Position = 0;
    fd->Public.Size = entry->Size;
    fd->FirstCluster = entry->FirstClusterLow + ((uint32_t)entry->FirstClusterHigh << 16);
    fd->CurrentCluster = fd->FirstCluster;
    fd->CurrentSectorInCluster = 0;

    if (!DiskReadSectors(disk, ClusterToLba(fd->CurrentCluster), 1, fd->Buffer))
    {
        printf("Fat: read open dir error\r\n");
        return NULL;
    }

    fd->Opened = true;
    return &fd->Public;
}

uint32_t FatNextCluster(uint32_t currentCluster)
{
    uint32_t fatIndex = currentCluster * 3 / 2;

    if (currentCluster % 2 == 0)
        return (*(uint16_t far *)(Fat + fatIndex)) & 0x0FFF;
    else
        return (*(uint16_t far *)(Fat + fatIndex)) >> 4;
}

uint32_t FatRead(Disk *disk, FatFile far *file, uint32_t byteCount, void *dataOut)
{
    FatFileData far *fd = (file->Handle == ROOT_DIRECTORY_HANDLE)
                              ? &Data->RootDirectory
                              : &Data->OpenedFiles[file->Handle];

    uint8_t *u8DataOut = (uint8_t *)dataOut;

    // don't read past the end of the file
    if (!fd->Public.IsDirectory)
        byteCount = min(byteCount, fd->Public.Size - fd->Public.Position);

    while (byteCount > 0)
    {
        uint32_t leftInBuffer = SECTOR_SIZE - (fd->Public.Position % SECTOR_SIZE);
        uint32_t take = min(byteCount, leftInBuffer);

        MemCpy(u8DataOut, fd->Buffer + fd->Public.Position % SECTOR_SIZE, take);
        u8DataOut += take;
        fd->Public.Position += take;
        byteCount -= take;

        // printf("leftInBuffer=%lu take=%lu\r\n", leftInBuffer, take);
        // See if we need to read more data
        if (leftInBuffer == take)
        {
            // Special handling for root directory
            if (fd->Public.Handle == ROOT_DIRECTORY_HANDLE)
            {
                ++fd->CurrentCluster;

                // read next sector
                if (!DiskReadSectors(disk, fd->CurrentCluster, 1, fd->Buffer))
                {
                    printf("FAT: read error!\r\n");
                    break;
                }
            }
            else
            {
                // calculate next cluster & sector to read
                if (++fd->CurrentSectorInCluster >= Data->BS.BootSector.SectorsPerCluster)
                {
                    fd->CurrentSectorInCluster = 0;
                    fd->CurrentCluster = FatNextCluster(fd->CurrentCluster);
                }

                if (fd->CurrentCluster >= 0xFF8)
                {
                    // Mark end of file
                    fd->Public.Size = fd->Public.Position;
                    break;
                }

                // read next sector
                if (!DiskReadSectors(disk,
                                     ClusterToLba(fd->CurrentCluster) + fd->CurrentSectorInCluster,
                                     1, fd->Buffer))
                {
                    printf("FAT: read error!\r\n");
                    break;
                }
            }
        }
    }

    return u8DataOut - (uint8_t *)dataOut;
}

bool FatReadEntry(Disk *disk, FatFile far *file, FatDirectoryEntry *dirEntry)
{
    return FatRead(disk, file, sizeof(FatDirectoryEntry), dirEntry) == sizeof(FatDirectoryEntry);
}

void FatClose(FatFile far *file)
{
    if (file->Handle == ROOT_DIRECTORY_HANDLE)
    {
        file->Position = 0;
        Data->RootDirectory.CurrentCluster = Data->RootDirectory.FirstCluster;
    }
    else
    {
        Data->OpenedFiles[file->Handle].Opened = false;
    }
}

bool FindFile(Disk *disk, FatFile far *file, const char *name, FatDirectoryEntry *entryOut)
{
    char fatName[12];
    FatDirectoryEntry entry;

    // convert from name to fat name
    MemSet(fatName, ' ', sizeof(fatName));
    fatName[11] = '\0';

    const char *ext = StrChr(name, '.');
    if (ext == NULL)
        ext = name + 11;

    for (int i = 0; i < 8 && name[i] && name + i < ext; i++)
        fatName[i] = ToUpper(name[i]);

    if (ext != NULL)
    {
        for (int i = 0; i < 3 && ext[i + 1]; i++)
            fatName[i + 8] = ToUpper(ext[i + 1]);
    }

    while (FatReadEntry(disk, file, &entry))
    {
        if (MemCmp(fatName, entry.Name, 11) == 0)
        {
            *entryOut = entry;
            return true;
        }
    }

    return false;
}

FatFile far *FatOpen(Disk *disk, const char *path)
{
    char name[MAX_PATH_SIZE];

    // ignore leading slash
    if (path[0] == '/')
        path++;

    FatFile far *current = &Data->RootDirectory.Public;

    while (*path)
    {
        // extract next file name from path
        bool isLast = false;
        const char *delim = StrChr(path, '/');
        if (delim != NULL)
        {
            MemCpy(name, path, delim - path);
            name[delim - path + 1] = '\0';
            path = delim + 1;
        }
        else
        {
            unsigned len = StrLen(path);
            MemCpy(name, path, len);
            name[len + 1] = '\0';
            path += len;
            isLast = true;
        }

        // find directory entry in current directory
        FatDirectoryEntry entry;
        if (FindFile(disk, current, name, &entry))
        {
            FatClose(current);

            // check if directory
            if (!isLast && (entry.Attributes & FAT_ATTRIBUTE_DIRECTORY) == 0)
            {
                printf("FAT: %s not a directory\r\n", name);
                return NULL;
            }

            // open new directory entry
            current = OpenDirEntry(disk, &entry);
        }
        else
        {
            FatClose(current);

            printf("FAT: %s not found\r\n", name);
            return NULL;
        }
    }

    return current;
}
