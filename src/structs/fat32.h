#ifndef FAT32_H
#define FAT32_H

#include <stdint.h>
#include <stdio.h>

#pragma pack(push, 1)
typedef struct {
    uint8_t jumpCode[3];
    char oemName[8];
    uint16_t bytesPerSector;
    uint8_t sectorsPerCluster;
    uint16_t reservedSectors;
    uint8_t fatCount;
    uint16_t rootEntryCount;
    uint16_t totalSectors16;
    uint8_t mediaType;
    uint16_t sectorsPerFat16;
    uint16_t sectorsPerTrack;
    uint16_t headCount;
    uint32_t hiddenSectors;
    uint32_t totalSectors32;
    uint32_t sectorsPerFat32;
    uint16_t extendedFlags;
    uint16_t fsVersion;
    uint32_t rootCluster;
    uint16_t fsInfo;
    uint16_t backupBootSector;
    uint8_t reserved[12];
    uint8_t driveNumber;
    uint8_t reserved1;
    uint8_t bootSignature;
    uint32_t volumeId;
    char volumeLabel[11];
    char fsType[8];
    uint8_t bootCode[420];
    uint16_t signature;
} BootSector;
#pragma pack(pop)

typedef struct {
    BootSector bootSector;
    FILE *image;
    uint32_t fatStart;
    uint32_t dataStart;
} FAT32Partition;

FILE* open_fat32_image(const char* image_path);
int read_boot_sector(FILE* image, BootSector* bootSector);
int mount_fat32(const char* image_path, FAT32Partition* partition);
void unmount_fat32(FAT32Partition* partition);
int fat32_read_cluster(FAT32Partition* partition, int cluster, uint8_t* buffer);
void fat32_print_info(FAT32Partition* partition);

#endif // FAT32_H