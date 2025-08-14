#include "disk.h"
#include "fat.h"
#include "stdint.h"
#include "stdio.h"

void far *g_data = (void far *)0x00500200;

void _cdecl cstart_(uint16_t bootDrive)
{
    Disk disk;
    if (!DiskInitialize(&disk, bootDrive))
    {
        printf("Disk init error\r\n");
        goto end;
    }

    DiskReadSectors(&disk, 19, 1, g_data);

    if (!FatInitialize(&disk))
    {
        printf("FAT init error\r\n");
        goto end;
    }

    // browse files in root
    FatFile far *fd = FatOpen(&disk, "/");
    FatDirectoryEntry entry;
    int i = 0;
    while (FatReadEntry(&disk, fd, &entry) && i++ < 5)
    {
        printf("  ");
        for (int i = 0; i < 11; i++)
            putc(entry.Name[i]);
        printf("\r\n");
    }
    FatClose(fd);

    char buffer[100];
    uint32_t read;
    fd = FatOpen(&disk, "test.txt");
    while ((read = FatRead(&disk, fd, sizeof(buffer), buffer)))
    {
        for (uint32_t i = 0; i < read; i++)
        {
            if (buffer[i] == '\n')
                putc('\r');
            putc(buffer[i]);
        }
    }
    FatClose(fd);

end:
    for (;;)
        ;
}
