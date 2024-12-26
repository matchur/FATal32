#include "fat32.h"
#include <stdlib.h>
#include <string.h>

// Carrega a partição FAT32 de uma imagem
int fat32_load_partition(FAT32Partition *partition, const char *imagePath) {
    partition->imageFile = fopen(imagePath, "rb+");
    if (!partition->imageFile) {
        perror("Erro ao abrir imagem FAT32");
        return -1;
    }

    // Lê o Boot Sector
    fseek(partition->imageFile, 0, SEEK_SET);
    fread(&partition->bootSector, sizeof(FAT32BootSector), 1, partition->imageFile);

    // Calcula o tamanho da FAT e aloca memória
    partition->fatTable.size = partition->bootSector.FATSize * 512 / sizeof(uint32_t);
    partition->fatTable.entries = (uint32_t *)malloc(partition->fatTable.size * sizeof(uint32_t));
    if (!partition->fatTable.entries) {
        perror("Erro ao alocar memória para a FAT");
        fclose(partition->imageFile);
        return -1;
    }

    // Lê a FAT
    fseek(partition->imageFile, partition->bootSector.reservedSectors * 512, SEEK_SET);
    fread(partition->fatTable.entries, sizeof(uint32_t), partition->fatTable.size, partition->imageFile);

    return 0;
}

// Libera recursos da partição
void fat32_free_partition(FAT32Partition *partition) {
    if (partition->imageFile) {
        fclose(partition->imageFile);
    }
    if (partition->fatTable.entries) {
        free(partition->fatTable.entries);
    }
}

// Lê o conteúdo de um cluster
int fat32_read_cluster(FAT32Partition *partition, uint32_t cluster, void *buffer) {
    if (cluster < 2 || cluster >= partition->fatTable.size) {
        fprintf(stderr, "Cluster inválido: %u\n", cluster);
        return -1;
    }

    uint32_t clusterOffset = (partition->bootSector.reservedSectors + partition->bootSector.FATSize * partition->bootSector.numberOfFATs) * 512;
    clusterOffset += (cluster - 2) * partition->bootSector.sectorsPerCluster * partition->bootSector.bytesPerSector;

    fseek(partition->imageFile, clusterOffset, SEEK_SET);
    fread(buffer, partition->bootSector.bytesPerSector, partition->bootSector.sectorsPerCluster, partition->imageFile);

    return 0;
}

// Escreve dados em um cluster
int fat32_write_cluster(FAT32Partition *partition, uint32_t cluster, const void *data) {
    if (cluster < 2 || cluster >= partition->fatTable.size) {
        fprintf(stderr, "Cluster inválido: %u\n", cluster);
        return -1;
    }

    uint32_t clusterOffset = (partition->bootSector.reservedSectors + partition->bootSector.FATSize * partition->bootSector.numberOfFATs) * 512;
    clusterOffset += (cluster - 2) * partition->bootSector.sectorsPerCluster * partition->bootSector.bytesPerSector;

    fseek(partition->imageFile, clusterOffset, SEEK_SET);
    fwrite(data, partition->bootSector.bytesPerSector, partition->bootSector.sectorsPerCluster, partition->imageFile);

    return 0;
}

// Obtém o próximo cluster na cadeia
int fat32_get_next_cluster(FAT32Partition *partition, uint32_t cluster) {
    if (cluster >= partition->fatTable.size) {
        fprintf(stderr, "Cluster fora do limite: %u\n", cluster);
        return -1;
    }
    return partition->fatTable.entries[cluster];
}

// Exibe informações sobre a partição
void fat32_print_info(const FAT32Partition *partition) {
    printf("FAT32 Info:\n");
    printf("OEM Name: %.8s\n", partition->bootSector.OEMName);
    printf("Bytes por setor: %u\n", partition->bootSector.bytesPerSector);
    printf("Setores por cluster: %u\n", partition->bootSector.sectorsPerCluster);
    printf("Total de setores: %u\n", partition->bootSector.totalSectors);
    printf("Cluster raiz: %u\n", partition->bootSector.rootCluster);
}
