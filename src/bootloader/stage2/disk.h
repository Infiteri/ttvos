#pragma once

#include "stdint.h"

typedef struct {
  uint8_t Id;
  uint32_t Cylinders;
  uint32_t Heads;
  uint32_t Sectors;
} Disk;

bool DiskInitialize(Disk *disk, uint8_t driveNumber);
bool DiskReadSectors(Disk *disk, uint32_t lba, uint8_t sectors,
                     void far *dataOut);
