#ifndef FAT32_H
#define FAT32_H

#include <stdint.h>
#include <stdio.h>

#define ATTR_DIRECTORY 0x10


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

#pragma pack(push, 1)
typedef struct {
    char name[11];
    uint8_t attributes;
    uint8_t reserved;
    uint8_t creationTimeTenths;
    uint16_t creationTime;
    uint16_t creationDate;
    uint16_t lastAccessDate;
    uint16_t firstClusterHigh;
    uint16_t writeTime;
    uint16_t writeDate;
    uint16_t firstClusterLow;
    uint32_t fileSize;
} DirectoryEntry;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct {
    uint8_t order;           // Número da sequência (bit 6 indica a última entrada)
    uint16_t name1[5];       // 5 caracteres (UTF-16)
    uint8_t attr;            // Deve ser 0x0F
    uint8_t type;            // Reservado (sempre 0)
    uint8_t checksum;
    uint16_t name2[6];       // 6 caracteres (UTF-16)
    uint16_t firstClusterLow; // Sempre 0 em entradas LFN
    uint16_t name3[2];       // 2 caracteres (UTF-16)
} LFNEntry;
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
uint32_t fat32_get_next_cluster(FAT32Partition *partition, uint32_t cluster);
void unmount_fat32(FAT32Partition* partition);
int fat32_read_cluster(FAT32Partition* partition, int cluster, uint8_t* buffer);
void fat32_print_info(FAT32Partition* partition);
int fat32_write_cluster(FAT32Partition* partition, int cluster, const uint8_t* buffer);
int fat32_allocate_cluster(FAT32Partition* partition);
int fat32_free_cluster_chain(FAT32Partition *partition, uint32_t startCluster);
int fat32_set_cluster(FAT32Partition *partition, uint32_t cluster, uint32_t value);


#endif // FAT32_H