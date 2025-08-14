#include "disk.h"
#include "stdint.h"
#include "x86.h"

bool DiskInitialize(Disk *disk, uint8_t driveNumber)
{
    uint8_t driveType;
    uint16_t cylinders, sectors, heads;

    if (!x86_Disk_GetDriveParams(driveNumber, &driveType, &cylinders, &sectors, &heads))
        return false;

    disk->Id = driveNumber;
    disk->Cylinders = cylinders + 1;
    disk->Sectors = sectors;
    disk->Heads = heads + 1;

    return true;
}

void LBA2CHS(Disk *disk, uint32_t lba, uint16_t *cOut, uint16_t *sOut, uint16_t *hOut)
{
    *sOut = lba % disk->Sectors + 1;
    *cOut = (lba / disk->Sectors) / disk->Heads;
    *hOut = (lba / disk->Sectors) % disk->Heads;
}

bool DiskReadSectors(Disk *disk, uint32_t lba, uint8_t sectors, void far *dataOut)
{
    uint16_t cylinder, sector, head;
    LBA2CHS(disk, lba, &cylinder, &sector, &head);

    for (int i = 0; i < 3; i++)
    {
        if (x86_Disk_Read(disk->Id, cylinder, sector, head, sectors, dataOut))
            return true;

        x86_Disk_Reset(disk->Id);
    }
    return false;
}
