#include "fat32.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Função para ler o setor de boot da imagem FAT32
int read_boot_sector(FILE* image, BootSector* bootSector) {
    if (fseek(image, 0, SEEK_SET) != 0) {
        fprintf(stderr, "Erro ao posicionar o ponteiro no início da imagem.\n");
        return -1;
    }
    if (fread(bootSector, sizeof(BootSector), 1, image) != 1) {
        fprintf(stderr, "Erro ao ler o setor de boot.\n");
        return -1;
    }
    return 0;
}

int mount_fat32(const char *image_path, FAT32Partition *partition) {
    FILE *image = fopen(image_path, "rb");
    if (image == NULL) {
        fprintf(stderr, "ERRO FATAL: Não foi possível abrir a imagem FAT32.\n");
        return -1;
    }

    // Ler o setor de boot
    if (read_boot_sector(image, &partition->bootSector) != 0) {
        fprintf(stderr, "ERRO FATAL: Erro ao ler o setor de boot.\n");
        fclose(image);
        return -1;
    }

    // Verificar a assinatura do setor de boot
    if (partition->bootSector.signature != 0xAA55) {
        fprintf(stderr, "ERRO FATAL: Assinatura do setor de boot inválida.\n");
        fclose(image);
        return -1;
    }

    // Inicializar outros campos da partição FAT32
    partition->image = image;
    partition->fatStart = partition->bootSector.reservedSectors;
    partition->dataStart = partition->fatStart + (partition->bootSector.fatCount * partition->bootSector.sectorsPerFat32);

    return 0;
}

void unmount_fat32(FAT32Partition *partition) {
    if (partition->image != NULL) {
        fclose(partition->image);
        partition->image = NULL;
    }
}

int fat32_read_cluster(FAT32Partition* partition, int cluster, uint8_t* buffer) {
    size_t clusterSize = partition->bootSector.bytesPerSector * partition->bootSector.sectorsPerCluster;
    size_t offset = (partition->bootSector.reservedSectors + partition->bootSector.fatCount * partition->bootSector.sectorsPerFat32) * partition->bootSector.bytesPerSector + (cluster - 2) * clusterSize;

    if (fseek(partition->image, offset, SEEK_SET) != 0) {
        return -1;
    }

    if (fread(buffer, clusterSize, 1, partition->image) != 1) {
        return -1;
    }

    return 0;
}

void fat32_print_info(FAT32Partition* partition) {
    printf("Informações da partição FAT32:\n");
    printf("Bytes por setor: %u\n", partition->bootSector.bytesPerSector);
    printf("Setores por cluster: %u\n", partition->bootSector.sectorsPerCluster);
    printf("Setores reservados: %u\n", partition->bootSector.reservedSectors);
    printf("Número de FATs: %u\n", partition->bootSector.fatCount);
    printf("Setores por FAT32: %u\n", partition->bootSector.sectorsPerFat32);
    printf("Assinatura do setor de boot: 0x%X\n", partition->bootSector.signature);
}