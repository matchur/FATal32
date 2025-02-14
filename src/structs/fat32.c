/*  
===============================================================================  
Nome do Projeto : FATal32 
Descrição       : Esse é o código fat32.c responsável pelas funções básicas do modelo
                  estrutural FAT32.
Autor           : Matheus V. Costa  
Data de Criação : 25/12/2024  
Última Alteração: 14/02/2024  
===============================================================================  
*/

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

uint32_t fat32_get_next_cluster(FAT32Partition *partition, uint32_t cluster) {
    // Verifica se o cluster é válido
    if (cluster < 2 || cluster >= 0x0FFFFFF8) {
        return 0x0FFFFFF8; // Fim da cadeia
    }

    // Calcula o offset da entrada na FAT
    size_t fatOffset = partition->fatStart * partition->bootSector.bytesPerSector;
    size_t entryOffset = fatOffset + (cluster * 4); // Cada entrada da FAT32 tem 4 bytes

    // Lê a entrada da FAT
    uint32_t nextCluster;
    if (fseek(partition->image, entryOffset, SEEK_SET) != 0) {
        fprintf(stderr, "Erro ao posicionar o ponteiro na FAT.\n");
        return 0x0FFFFFF8; // Fim da cadeia em caso de erro
    }
    if (fread(&nextCluster, sizeof(uint32_t), 1, partition->image) != 1) {
        fprintf(stderr, "Erro ao ler a entrada da FAT.\n");
        return 0x0FFFFFF8; // Fim da cadeia em caso de erro
    }

    // A FAT32 usa apenas 28 bits para o número do cluster
    return nextCluster & 0x0FFFFFFF;
}

int fat32_set_cluster(FAT32Partition *partition, uint32_t cluster, uint32_t value) {
    // Verifica se o cluster é válido
    if (cluster < 2 || cluster >= 0x0FFFFFF8) {
        fprintf(stderr, "Cluster inválido: %u\n", cluster);
        return -1;
    }

    // Calcula o offset da entrada na FAT
    size_t fatOffset = partition->fatStart * partition->bootSector.bytesPerSector;
    size_t entryOffset = fatOffset + (cluster * 4); // Cada entrada da FAT32 tem 4 bytes

    // Define o valor do cluster (usando apenas 28 bits)
    value &= 0x0FFFFFFF;

    // Atualiza todas as cópias da FAT
    for (uint32_t i = 0; i < partition->bootSector.fatCount; i++) {
        size_t currentFatOffset = fatOffset + (i * partition->bootSector.sectorsPerFat32 * partition->bootSector.bytesPerSector);

        // Posiciona o ponteiro na entrada correta
        if (fseek(partition->image, currentFatOffset + (cluster * 4), SEEK_SET) != 0) {
            fprintf(stderr, "Erro ao posicionar o ponteiro na FAT %u.\n", i);
            return -1;
        }

        // Escreve o valor na FAT
        if (fwrite(&value, sizeof(uint32_t), 1, partition->image) != 1) {
            fprintf(stderr, "Erro ao escrever na FAT %u.\n", i);
            return -1;
        }
    }

    // Garante que os dados sejam gravados imediatamente
    fflush(partition->image);

    return 0;
}

int fat32_free_cluster_chain(FAT32Partition *partition, uint32_t startCluster) {
    while (startCluster < 0x0FFFFFF8) { // 0x0FFFFFF8 é o marcador de fim de cadeia
        uint32_t nextCluster = fat32_get_next_cluster(partition, startCluster);
        fat32_set_cluster(partition, startCluster, 0); // Marca o cluster como livre
        startCluster = nextCluster;
    }
    return 0;
}

int fat32_allocate_cluster(FAT32Partition* partition) {
    // Obtém parâmetros do BootSector.
    uint32_t bytesPerSector = partition->bootSector.bytesPerSector;
    uint32_t sectorsPerFat32 = partition->bootSector.sectorsPerFat32;
    uint32_t fatCount = partition->bootSector.fatCount;
    
    // Calcula o tamanho da FAT em bytes.
    size_t fatSize = sectorsPerFat32 * bytesPerSector;

    // Calcula o offset da primeira FAT na imagem.
    size_t fatOffset = partition->fatStart * bytesPerSector;

    // Aloca buffer para a FAT.
    uint8_t *fatBuffer = malloc(fatSize);
    if (!fatBuffer) {
        fprintf(stderr, "Erro: sem memória para alocar FAT buffer.\n");
        return -1;
    }

    // Lê a primeira FAT da imagem.
    if (fseek(partition->image, fatOffset, SEEK_SET) != 0) {
        fprintf(stderr, "Erro: não foi possível posicionar o ponteiro para ler a FAT.\n");
        free(fatBuffer);
        return -1;
    }
    if (fread(fatBuffer, fatSize, 1, partition->image) != 1) {
        fprintf(stderr, "Erro: não foi possível ler a FAT.\n");
        free(fatBuffer);
        return -1;
    }

    // A FAT32 utiliza entradas de 32 bits. Embora apenas 28 bits sejam efetivos,
    // vamos tratar cada entrada como um uint32_t.
    uint32_t *fatEntries = (uint32_t*)fatBuffer;

    // Calcular o número total de clusters.
    // O número total de setores da partição está em totalSectors32.
    uint32_t totalSectors = partition->bootSector.totalSectors32;
    uint32_t reservedSectors = partition->bootSector.reservedSectors;
    uint32_t fatSectors = fatCount * sectorsPerFat32;
    uint32_t dataSectors = totalSectors - reservedSectors - fatSectors;
    uint32_t totalClusters = dataSectors / partition->bootSector.sectorsPerCluster;
    // Os clusters válidos começam em 2.
    
    // Procura um cluster livre: uma entrada com valor zero.
    int allocatedCluster = -1;
    for (uint32_t cluster = 2; cluster < 2 + totalClusters; cluster++) {
        // Se os 28 bits inferiores estiverem zerados, o cluster está livre.
        if ((fatEntries[cluster] & 0x0FFFFFFF) == 0) {
            allocatedCluster = cluster;
            // Marca o cluster como alocado, usando o marcador de fim de cadeia.
            fatEntries[cluster] = 0x0FFFFFFF;
            break;
        }
    }

    if (allocatedCluster == -1) {
        fprintf(stderr, "Erro: Não há clusters livres disponíveis.\n");
        free(fatBuffer);
        return -1;
    }

    // Atualiza todas as cópias da FAT na imagem.
    // Cada FAT está localizada em sequência, começando em fatOffset.
    for (uint32_t i = 0; i < fatCount; i++) {
        size_t currentFatOffset = (partition->bootSector.reservedSectors + i * sectorsPerFat32) * bytesPerSector;
        if (fseek(partition->image, currentFatOffset, SEEK_SET) != 0) {
            fprintf(stderr, "Erro: não foi possível posicionar o ponteiro para atualizar a FAT %u.\n", i);
            free(fatBuffer);
            return -1;
        }
        if (fwrite(fatBuffer, fatSize, 1, partition->image) != 1) {
            fprintf(stderr, "Erro: não foi possível escrever na FAT %u.\n", i);
            free(fatBuffer);
            return -1;
        }
        fflush(partition->image);
    }

    free(fatBuffer);
    return allocatedCluster;
}

int fat32_write_cluster(FAT32Partition* partition, int cluster, const uint8_t* buffer) {
    // Calcula o tamanho de um cluster.
    size_t clusterSize = partition->bootSector.bytesPerSector * partition->bootSector.sectorsPerCluster;
    // Calcula o offset do início dos clusters de dados.
    size_t dataOffset = (partition->bootSector.reservedSectors +
                         partition->bootSector.fatCount * partition->bootSector.sectorsPerFat32) *
                         partition->bootSector.bytesPerSector;
    // Em FAT32 os clusters começam em 2.
    size_t offset = dataOffset + (cluster - 2) * clusterSize;
    
    // Posiciona o ponteiro de arquivo para o offset calculado.
    if (fseek(partition->image, offset, SEEK_SET) != 0) {
        return -1;
    }
    
    // Escreve o buffer no cluster.
    if (fwrite(buffer, clusterSize, 1, partition->image) != 1) {
        return -1;
    }
    
    // Garante que os dados sejam gravados imediatamente.
    fflush(partition->image);
    
    return 0;
}

int mount_fat32(const char *image_path, FAT32Partition *partition) {
    FILE *image = fopen(image_path, "rb+");
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