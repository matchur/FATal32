#ifndef FAT32_H
#define FAT32_H

#include <stdint.h>
#include <stdio.h>

#define FAT32_ATTR_READ_ONLY  0x01
#define FAT32_ATTR_HIDDEN     0x02
#define FAT32_ATTR_SYSTEM     0x04
#define FAT32_ATTR_VOLUME_ID  0x08
#define FAT32_ATTR_DIRECTORY  0x10
#define FAT32_ATTR_ARCHIVE    0x20

// Estrutura para representar o Boot Sector da FAT32
typedef struct {
    char OEMName[8];           // Nome OEM
    uint16_t bytesPerSector;   // Bytes por setor
    uint8_t sectorsPerCluster; // Setores por cluster
    uint16_t reservedSectors;  // Setores reservados
    uint8_t numberOfFATs;      // Número de FATs
    uint32_t totalSectors;     // Total de setores
    uint32_t FATSize;          // Tamanho da FAT em setores
    uint32_t rootCluster;      // Cluster raiz
} FAT32BootSector;

typedef struct {
    char name[11];              // Nome do arquivo/diretório (8.3 format)
    uint8_t attributes;         // Atributos (ex: arquivo, diretório, oculto, etc.)
    uint8_t reserved;           // Reservado para uso futuro
    uint8_t creationTimeTenth;  // Milissegundos da criação
    uint16_t creationTime;      // Hora da criação
    uint16_t creationDate;      // Data da criação
    uint16_t lastAccessDate;    // Data do último acesso
    uint16_t firstClusterHigh;  // Parte alta do cluster inicial
    uint16_t lastWriteTime;     // Hora da última modificação
    uint16_t lastWriteDate;     // Data da última modificação
    uint16_t firstClusterLow;   // Parte baixa do cluster inicial
    uint32_t fileSize;          // Tamanho do arquivo em bytes
} FAT32DirectoryEntry;

// Estrutura para representar a FAT em si
typedef struct {
    uint32_t *entries;         // Entradas da FAT (ex.: ponteiros para clusters)
    size_t size;               // Tamanho da FAT (em clusters)
} FAT32Table;

// Estrutura para representar a partição FAT32
typedef struct {
    FILE *imageFile;           // Ponteiro para o arquivo de imagem da partição
    FAT32BootSector bootSector; // Boot Sector
    FAT32Table fatTable;        // Tabela FAT
} FAT32Partition;

// Funções para inicializar e liberar a partição
int fat32_load_partition(FAT32Partition *partition, const char *imagePath);
void fat32_free_partition(FAT32Partition *partition);

// Operações básicas
int fat32_read_cluster(FAT32Partition *partition, uint32_t cluster, void *buffer);
int fat32_write_cluster(FAT32Partition *partition, uint32_t cluster, const void *data);
int fat32_get_next_cluster(FAT32Partition *partition, uint32_t cluster);

// Helpers
void fat32_print_info(const FAT32Partition *partition);

#endif // FAT32_H
