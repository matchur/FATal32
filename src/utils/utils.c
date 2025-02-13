#include "utils.h"
#include <unistd.h>

bool file_exists(const char *path) {
    return access(path, F_OK) == 0;
}

uint8_t calc_lfn_checksum(const char shortName[11]) {
    uint8_t sum = 0;
    for (int i = 0; i < 11; i++) {
        sum = ((sum & 1) ? 0x80 : 0) + (sum >> 1) + shortName[i];
    }
    return sum;
}

// Função auxiliar para extrair os caracteres de uma entrada LFN e retorná-los em um buffer (ASCII)
void lfn_extract_part(const LFNEntry *lfn, char *dest, size_t maxDestSize) {
    size_t pos = 0;
    #define EXTRACT(arr, count) \
        for (int i = 0; i < (count); i++) { \
            uint16_t wc = lfn->arr[i]; \
            if (wc == 0x0000 || wc == 0xFFFF) break; \
            if (pos < maxDestSize - 1) { \
                dest[pos++] = (char)(wc & 0xFF); \
            } \
        }
    // Extraindo as três partes
    for (int i = 0; i < 5; i++) {
        uint16_t wc = lfn->name1[i];
        if (wc == 0x0000 || wc == 0xFFFF) break;
        if (pos < maxDestSize - 1) {
            dest[pos++] = (char)(wc & 0xFF);
        }
    }
    for (int i = 0; i < 6; i++) {
        uint16_t wc = lfn->name2[i];
        if (wc == 0x0000 || wc == 0xFFFF) break;
        if (pos < maxDestSize - 1) {
            dest[pos++] = (char)(wc & 0xFF);
        }
    }
    for (int i = 0; i < 2; i++) {
        uint16_t wc = lfn->name3[i];
        if (wc == 0x0000 || wc == 0xFFFF) break;
        if (pos < maxDestSize - 1) {
            dest[pos++] = (char)(wc & 0xFF);
        }
    }
    dest[pos] = '\0';
    #undef EXTRACT
}

/// Converte uma string para maiúsculas.
void str_to_upper(char *str) {
    for (int i = 0; str[i]; i++) {
        str[i] = toupper(str[i]);
    }
}

/// Formata uma string para o formato curto 8.3 (11 bytes)  
/// Recebe "newname" (ex: "OTHER_TXTFILE.TXT") e produz um array de 11 bytes, sem separador de ponto.
/// Se a parte base for menor que 8 caracteres ou a extensão menor que 3, preenche com espaços.
void format_short_name(const char *newname, char dest[11]) {
    char base[9] = {0};
    char ext[4] = {0};

    // Faz uma cópia do novo nome para tokenização.
    char temp[256];
    strncpy(temp, newname, sizeof(temp) - 1);
    temp[sizeof(temp) - 1] = '\0';

    // Converte para maiúsculas (o padrão FAT utiliza maiúsculas).
    str_to_upper(temp);

    // Separa a parte base e a extensão (usando o primeiro ponto encontrado).
    char *dot = strchr(temp, '.');
    if (dot) {
        *dot = '\0';
        strncpy(base, temp, 8);
        strncpy(ext, dot + 1, 3);
    } else {
        strncpy(base, temp, 8);
    }

    // Preenche com espaços se necessário.
    for (int i = 0; i < 8; i++) {
        if (i >= (int)strlen(base))
            base[i] = ' ';
    }
    for (int i = 0; i < 3; i++) {
        if (i >= (int)strlen(ext))
            ext[i] = ' ';
    }

    memcpy(dest, base, 8);
    memcpy(dest + 8, ext, 3);
}


void recursive_delete_dir(FAT32Partition *partition, uint32_t cluster, const char *basePath) {
    size_t clusterSize = partition->bootSector.bytesPerSector * partition->bootSector.sectorsPerCluster;
    uint8_t *buffer = malloc(clusterSize);
    if (!buffer) {
        printf("Erro ao alocar memória.\n");
        return;
    }

    if (fat32_read_cluster(partition, cluster, buffer) != 0) {
        printf("Erro ao ler o cluster do diretório.\n");
        free(buffer);
        return;
    }

    DirectoryEntry *entry = (DirectoryEntry *)buffer;
    char currentDir[MAX_PATH_LEN];
    strncpy(currentDir, basePath, sizeof(currentDir) - 1);
    currentDir[sizeof(currentDir) - 1] = '\0';  // Garante terminação

    char lfnBuffer[MAX_LFN_LEN] = {0};  // Buffer para armazenar nome longo
    int lfnIndex = 0;

    // Processa todas as entradas
    for (size_t i = 0; i < clusterSize / sizeof(DirectoryEntry); i++) {
        if (entry[i].name[0] == 0x00) break;
        if ((unsigned char)entry[i].name[0] == 0xE5) continue;
        if ((entry[i].attributes & 0x0F) == 0x0F) { 
            extract_lfn(entry[i], lfnBuffer, &lfnIndex);
            continue;
        }

        // Ignora "." e ".."
        if (strncmp(entry[i].name, ".          ", 11) == 0 || 
            strncmp(entry[i].name, "..         ", 11) == 0) {
            lfnBuffer[0] = '\0';
            lfnIndex = 0;
            continue;
        }

        // Obtém o nome correto (LFN ou SFN)
        char name[MAX_LFN_LEN];
        if (lfnBuffer[0] != '\0') {
            strncpy(name, lfnBuffer, sizeof(name) - 1);
            name[sizeof(name) - 1] = '\0';
        } else {
            extract_sfn(entry[i], name);
        }

        lfnBuffer[0] = '\0';
        lfnIndex = 0;

        // Constrói o caminho completo
        char fullPath[MAX_PATH_LEN];
        snprintf(fullPath, sizeof(fullPath), "%s/%s", basePath, name);
        fullPath[sizeof(fullPath) - 1] = '\0';  // Garante terminação

        if (!(entry[i].attributes & 0x10)) {
            printf("Removendo arquivo: %s\n", fullPath);
            cmd_rm(fullPath);
        } else {
            uint32_t subCluster = (entry[i].firstClusterHigh << 16) | entry[i].firstClusterLow;
            printf("Entrando no diretório: %s\n", fullPath);
            recursive_delete_dir(partition, subCluster, fullPath);
        }
    }

    // Após excluir tudo, remove o próprio diretório
    printf("Removendo diretório: %s\n", basePath);
    fat32_free_cluster_chain(partition, cluster);
    free(buffer);
}


int is_directory_empty(FAT32Partition *partition, uint32_t cluster) {
    size_t clusterSize = partition->bootSector.bytesPerSector * partition->bootSector.sectorsPerCluster;
    uint8_t *buffer = malloc(clusterSize);
    if (!buffer) {
        printf("Erro ao alocar memória.\n");
        return 0;
    }

    if (fat32_read_cluster(partition, cluster, buffer) != 0) {
        printf("Erro ao ler o cluster do diretório.\n");
        free(buffer);
        return 0;
    }

    DirectoryEntry *entry = (DirectoryEntry *)buffer;
    int isEmpty = 1;

    for (size_t i = 0; i < clusterSize / sizeof(DirectoryEntry); i++) {
        if (entry[i].name[0] == 0x00) break; // Fim das entradas
        if ((unsigned char)entry[i].name[0] == 0xE5) continue; // Entrada deletada
        if ((entry[i].attributes & 0x0F) == 0x0F) continue; // Entrada LFN

        // Ignora "." e ".."
        if (strncmp(entry[i].name, ".          ", 11) == 0 || 
            strncmp(entry[i].name, "..         ", 11) == 0) {
            continue;
        }

        isEmpty = 0;
        break;
    }

    free(buffer);
    return isEmpty;
}


int update_lfn_entries(uint8_t *buffer, int mainIndex, const char *newname) {
    // Primeiro, conte quantas entradas LFN existem imediatamente antes da entrada principal.
    int lfnCount = 0;
    int idx = mainIndex - 1;
    while (idx >= 0) {
        DirectoryEntry *entry = (DirectoryEntry *)&buffer[idx * sizeof(DirectoryEntry)];
        // Verifica se o atributo é 0x0F (LFN)
        if ((entry->attributes & 0x0F) == 0x0F)
            lfnCount++;
        else
            break;
        idx--;
    }

    // Convertemos o novo nome para "wide" (supondo ASCII – cada caractere em 16 bits).
    int len = strlen(newname);
    uint16_t *newNameWide = malloc((len + 1) * sizeof(uint16_t));
    if (!newNameWide) {
        printf("Erro ao alocar memória para LFN.\n");
        return -1;
    }
    for (int i = 0; i < len; i++) {
        newNameWide[i] = (uint16_t)newname[i];
    }
    newNameWide[len] = 0;

    // Cada entrada LFN pode armazenar até 13 caracteres.
    int required = (len + 12) / 13;  // arredonda para cima
    if (required > lfnCount) {
        printf("Erro: Novo nome muito longo para as entradas LFN existentes (%d disponíveis, mas são necessários %d).\n", lfnCount, required);
        free(newNameWide);
        return -1;
    }

    // Para atualizar, vamos sobrescrever as últimas 'required' entradas LFN.
    // Lembre-se: as entradas LFN são armazenadas em ordem reversa. Assim,
    // a entrada imediatamente acima da entrada principal conterá os primeiros 13 caracteres do nome,
    // a anterior conterá os próximos 13 e assim por diante.
    for (int i = 0; i < required; i++) {
        int entryIndex = mainIndex - required + i;  // posição da i-ésima entrada LFN a ser atualizada
        LFNEntry *lfn = (LFNEntry *)&buffer[entryIndex * sizeof(DirectoryEntry)];
        // O campo order: a entrada com maior número (a última na sequência) tem o bit 0x40 definido.
        int orderVal = required - i;  // se i==0, entryIndex = mainIndex-required e este é o último (deve ter 0x40)
        if (i == 0)
            orderVal |= 0x40;
        lfn->order = (uint8_t)orderVal;

        // Preenche os campos de nome: name1 (5), name2 (6) e name3 (2) totalizando 13 caracteres.
        int start = (required - 1 - i) * 13; // posição no novo nome (os blocos são preenchidos do final para o início)
        // name1 (5 caracteres)
        for (int j = 0; j < 5; j++) {
            int pos = start + j;
            if (pos < len)
                lfn->name1[j] = newNameWide[pos];
            else
                lfn->name1[j] = 0xFFFF;  // 0xFFFF indica fim ou espaço vazio
        }
        // name2 (6 caracteres)
        for (int j = 0; j < 6; j++) {
            int pos = start + 5 + j;
            if (pos < len)
                lfn->name2[j] = newNameWide[pos];
            else
                lfn->name2[j] = 0xFFFF;
        }
        // name3 (2 caracteres)
        for (int j = 0; j < 2; j++) {
            int pos = start + 11 + j;
            if (pos < len)
                lfn->name3[j] = newNameWide[pos];
            else
                lfn->name3[j] = 0xFFFF;
        }
        lfn->attr = 0x0F;
        lfn->type = 0;
        // O campo firstClusterLow em entradas LFN deve ser zero.
        lfn->firstClusterLow = 0;
        // O checksum será calculado posteriormente (depois de atualizar a entrada curta).
    }

    free(newNameWide);
    return required; // retorna quantas entradas LFN foram atualizadas
}

void extract_sfn(DirectoryEntry entry, char *name) {
    memcpy(name, entry.name, 11);
    name[11] = '\0';
    
    // Remove espaços extras
    for (int j = 10; j >= 0 && name[j] == ' '; j--) name[j] = '\0';
}

void extract_lfn(DirectoryEntry entry, char *lfnBuffer, int *lfnIndex) {
    LFNEntry *lfn = (LFNEntry *)&entry;
    char temp[14];

    memcpy(temp, lfn->name1, 10);
    memcpy(temp + 10, lfn->name2, 12);
    memcpy(temp + 22, lfn->name3, 4);
    
    temp[26] = '\0';

    // Insere no início do buffer (LFNs são armazenados de trás para frente)
    memmove(lfnBuffer + strlen(temp), lfnBuffer, strlen(lfnBuffer) + 1);
    memcpy(lfnBuffer, temp, strlen(temp));

    *lfnIndex += 1;
}

int find_file_cluster(const char *path) {
    // 1. Separa o caminho em diretório e nome do arquivo
    char *pathCopy = strdup(path);
    if (!pathCopy) {
        printf("Erro ao alocar memória.\n");
        return -1;
    }

    char *dirPath = dirname(pathCopy); // Extrai o diretório
    char *fileName = basename(pathCopy); // Extrai o nome do arquivo

    // 2. Encontra o cluster do diretório
    int dirCluster = find_directory_cluster(dirPath);
    if (dirCluster < 0) {
        free(pathCopy);
        printf("Erro: Diretório não encontrado.\n");
        return -1;
    }

    // 3. Lê o cluster do diretório
    size_t clusterSize = root->bootSector.bytesPerSector * root->bootSector.sectorsPerCluster;
    uint8_t *buffer = malloc(clusterSize);
    if (!buffer) {
        free(pathCopy);
        printf("Erro ao alocar memória.\n");
        return -1;
    }

    if (fat32_read_cluster(root, dirCluster, buffer) != 0) {
        free(pathCopy);
        free(buffer);
        printf("Erro ao ler o diretório.\n");
        return -1;
    }

    // 4. Procura o arquivo no diretório
    DirectoryEntry *entry = (DirectoryEntry *)buffer;
    int foundCluster = -1;

    for (size_t i = 0; i < clusterSize / sizeof(DirectoryEntry); i++) {
        if (entry[i].name[0] == 0x00) break; // Fim das entradas
        if ((unsigned char)entry[i].name[0] == 0xE5) continue; // Entrada deletada
        if ((entry[i].attributes & 0x0F) == 0x0F) continue; // Entrada LFN

        // Extrai o nome curto (8.3)
        char name[9];
        memcpy(name, entry[i].name, 8);
        name[8] = '\0';
        for (int j = 7; j >= 0 && name[j] == ' '; j--) name[j] = '\0';

        // Converte para minúsculas
        for (int j = 0; name[j]; j++) name[j] = tolower(name[j]);

        // Converte o nome do arquivo para minúsculas
        char lower_fileName[256];
        strncpy(lower_fileName, fileName, 255);
        for (int j = 0; lower_fileName[j]; j++) lower_fileName[j] = tolower(lower_fileName[j]);

        // Compara o nome da entrada com o arquivo procurado
        if (strcmp(name, lower_fileName) == 0 && !(entry[i].attributes & 0x10)) {
            foundCluster = (entry[i].firstClusterHigh << 16) | entry[i].firstClusterLow;
            break;
        }
    }

    free(pathCopy);
    free(buffer);
    return foundCluster;
}

// Lê todo o conteúdo de um arquivo da imagem
int read_file_from_image(int startCluster, uint8_t **buffer, size_t *size) {
    uint32_t cluster = startCluster;
    size_t clusterSize = root->bootSector.bytesPerSector * root->bootSector.sectorsPerCluster;
    uint8_t *content = NULL;
    size_t totalSize = 0;
    
    while (cluster < 0x0FFFFFF8) {
        uint8_t *clusterData = malloc(clusterSize);
        if (!clusterData) {
            free(content);
            return -1;
        }
        
        if (fat32_read_cluster(root, cluster, clusterData) != 0) {
            free(clusterData);
            free(content);
            return -1;
        }
        
        content = realloc(content, totalSize + clusterSize);
        memcpy(content + totalSize, clusterData, clusterSize);
        totalSize += clusterSize;
        free(clusterData);
        
        cluster = fat32_get_next_cluster(root, cluster);
    }
    
    *buffer = content;
    *size = totalSize;
    return 0;
}

int write_file_to_image(const char *path, uint8_t *data, size_t size) {
    // 1. Separa o caminho em diretório e nome do arquivo
    char *pathCopy = strdup(path);
    if (!pathCopy) {
        printf("Erro ao alocar memória.\n");
        return -1;
    }

    char *dirPath = dirname(pathCopy); // Extrai o diretório
    char *fileName = basename(pathCopy); // Extrai o nome do arquivo

    // 2. Encontra o cluster do diretório
    int dirCluster = find_directory_cluster(dirPath);
    if (dirCluster < 0) {
        free(pathCopy);
        printf("Erro: Diretório não encontrado.\n");
        return -1;
    }

    // 3. Lê o cluster do diretório
    size_t clusterSize = root->bootSector.bytesPerSector * root->bootSector.sectorsPerCluster;
    uint8_t *buffer = malloc(clusterSize);
    if (!buffer) {
        free(pathCopy);
        printf("Erro ao alocar memória.\n");
        return -1;
    }

    if (fat32_read_cluster(root, dirCluster, buffer) != 0) {
        free(pathCopy);
        free(buffer);
        printf("Erro ao ler o diretório.\n");
        return -1;
    }

    // 4. Procura uma entrada livre no diretório
    DirectoryEntry *entry = (DirectoryEntry *)buffer;
    int freeIndex = -1;

    for (size_t i = 0; i < clusterSize / sizeof(DirectoryEntry); i++) {
        if (entry[i].name[0] == 0x00 || (unsigned char)entry[i].name[0] == 0xE5) {
            freeIndex = i;
            break;
        }
    }

    if (freeIndex == -1) {
        free(pathCopy);
        free(buffer);
        printf("Erro: Não há espaço livre no diretório.\n");
        return -1;
    }

    // 5. Cria a nova entrada de arquivo
    DirectoryEntry newEntry;
    memset(&newEntry, 0, sizeof(DirectoryEntry));

    // Formata o nome curto (8.3)
    format_short_name(fileName, newEntry.name);

    // Define atributos (arquivo comum)
    newEntry.attributes = 0x20;

    // Aloca clusters para o arquivo
    uint32_t startCluster = fat32_allocate_cluster(root);
    if (startCluster < 0) {
        free(pathCopy);
        free(buffer);
        printf("Erro ao alocar clusters.\n");
        return -1;
    }

    newEntry.firstClusterHigh = (uint16_t)(startCluster >> 16);
    newEntry.firstClusterLow = (uint16_t)(startCluster & 0xFFFF);
    newEntry.fileSize = size;

    // 6. Escreve a nova entrada no diretório
    memcpy(&entry[freeIndex], &newEntry, sizeof(DirectoryEntry));

    if (fat32_write_cluster(root, dirCluster, buffer) != 0) {
        free(pathCopy);
        free(buffer);
        printf("Erro ao escrever no diretório.\n");
        return -1;
    }

    // 7. Escreve os dados nos clusters alocados
    uint32_t currentCluster = startCluster;
    size_t bytesWritten = 0;

    while (bytesWritten < size) {
        size_t bytesToWrite = (size - bytesWritten > clusterSize) ? clusterSize : size - bytesWritten;

        if (fat32_write_cluster(root, currentCluster, data + bytesWritten) != 0) {
            printf("Erro ao escrever dados no cluster.\n");
            free(pathCopy);
            free(buffer);
            return -1;
        }

        bytesWritten += bytesToWrite;

        // Aloca próximo cluster se necessário
        if (bytesWritten < size) {
            uint32_t nextCluster = fat32_allocate_cluster(root);
            if (nextCluster < 0) {
                printf("Erro ao alocar próximo cluster.\n");
                free(pathCopy);
                free(buffer);
                return -1;
            }

            fat32_set_cluster(root, currentCluster, nextCluster);
            currentCluster = nextCluster;
        } else {
            fat32_set_cluster(root, currentCluster, 0x0FFFFFFF); // Fim da cadeia
        }
    }

    free(pathCopy);
    free(buffer);
    return 0;
}

void normalize_path(char *path) {
    int i = 0, j = 0;
    while (path[i]) {
        if (path[i] == '/' && path[i + 1] == '/') {
            i++;
            continue;
        }
        path[j++] = path[i++];
    }
    path[j] = '\0';
}

char* get_filename(const char *path) {
    char *filename = strrchr(path, '/');
    return filename ? filename + 1 : (char*)path;
}

int is_directory(FAT32Partition *partition, const char *path) {
    int cluster = find_directory_cluster(path);
    if (cluster < 0) return 0;
    
    size_t clusterSize = partition->bootSector.bytesPerSector * partition->bootSector.sectorsPerCluster;
    uint8_t *buffer = malloc(clusterSize);
    if (!buffer) return 0;
    
    if (fat32_read_cluster(partition, cluster, buffer) != 0) {
        free(buffer);
        return 0;
    }
    
    DirectoryEntry *entry = (DirectoryEntry*)buffer;
    int isDir = (entry[0].attributes & 0x10) ? 1 : 0;
    free(buffer);
    return isDir;
}


int read_internal_file(const char *path, uint8_t **data, size_t *size) {
    if (!path || !data || !size) {
        printf("Erro: parâmetros inválidos para read_internal_file().\n");
        return -1;
    }
    
    // Procura o cluster do arquivo usando o caminho interno.
    int cluster = find_file_cluster(path);
    if (cluster < 0) {
        printf("Erro: arquivo '%s' não encontrado.\n", path);
        return -1;
    }
    
    // Lê o arquivo a partir do cluster inicial.
    // Essa função já deve estar preparada para percorrer a cadeia de clusters.
    if (read_file_from_image(cluster, data, size) != 0) {
        printf("Erro ao ler o arquivo '%s' da imagem.\n", path);
        return -1;
    }
    
    printf("[DEBUG] read_internal_file: arquivo '%s' lido com sucesso (%zu bytes).\n", path, *size);
    return 0;
}

int write_internal_file(const char *path, uint8_t *data, size_t size) {
    if (!path || !data) {
        printf("Erro: parâmetros inválidos para write_internal_file().\n");
        return -1;
    }
    
    // Extrai o diretório pai do caminho.
    // Suponha que a função get_parent_directory() retorne um buffer alocado
    // contendo o caminho absoluto do diretório pai.
    char *parentDir = get_parent_directory(path);
    if (!parentDir) {
        printf("Erro: não foi possível obter o diretório pai para '%s'.\n", path);
        return -1;
    }
    
    printf("[DEBUG] write_internal_file: diretório pai para '%s' é '%s'.\n", path, parentDir);
    
    // Verifica (ou cria) o diretório pai na imagem.
    if (create_directory(parentDir) != 0) {
        printf("Erro ao criar ou localizar o diretório pai '%s'.\n", parentDir);
        free(parentDir);
        return -1;
    }
    free(parentDir);
    
    // Escreve o arquivo usando a implementação similar à write_file_to_image.
    if (write_file_to_image(path, data, size) != 0) {
        printf("Erro ao escrever o arquivo '%s' na imagem.\n", path);
        return -1;
    }
    
    printf("[DEBUG] write_internal_file: arquivo '%s' escrito com sucesso (%zu bytes).\n", path, size);
    return 0;
}

char *get_parent_directory(const char *path) {
    if (!path) return NULL;
    char *copy = strdup(path);
    if (!copy) return NULL;
    char *dir = dirname(copy);  // dirname modifica o buffer
    char *result = strdup(dir);
    free(copy);
    return result;
}

char *to_absolute_path(const char *path) {
    char *absPath = malloc(MAX_PATH_LEN);
    if (!absPath) {
        printf("Erro ao alocar memória em to_absolute_path().\n");
        return NULL;
    }
    if (path[0] == '/') {
        // Já é absoluto
        strncpy(absPath, path, MAX_PATH_LEN - 1);
        absPath[MAX_PATH_LEN - 1] = '\0';
    } else {
        // Se o caminho começar com "img/", remove o prefixo.
        if (strncmp(path, "img/", 4) == 0) {
            // Se current_path é a raiz, o resultado será "/<resto>".
            if (strcmp(current_path, "/") == 0)
                snprintf(absPath, MAX_PATH_LEN, "/%s", path + 4);
            else
                snprintf(absPath, MAX_PATH_LEN, "%s/%s", current_path, path + 4);
        } else {
            // Caminho relativo ao current_path.
            if (strcmp(current_path, "/") == 0)
                snprintf(absPath, MAX_PATH_LEN, "/%s", path);
            else
                snprintf(absPath, MAX_PATH_LEN, "%s/%s", current_path, path);
        }
    }
    // Normaliza: remove barras duplicadas.
    for (int i = 0; absPath[i]; i++) {
        if (absPath[i] == '/' && absPath[i + 1] == '/') {
            memmove(&absPath[i], &absPath[i + 1], strlen(&absPath[i + 1]) + 1);
            i--;
        }
    }
    return absPath;
}

// Função create_directory:
// Garante que o diretório representado por 'path' exista na imagem,
// criando recursivamente os diretórios que não existem.
// Retorna 0 em caso de sucesso ou -1 em caso de erro.
int create_directory(const char *path) {
    if (!path) {
        printf("create_directory: caminho inválido.\n");
        return -1;
    }
    
    // Converte o caminho para absoluto (se ainda não for)
    char absPath[MAX_PATH_LEN];
    if (path[0] == '/') {
        strncpy(absPath, path, MAX_PATH_LEN - 1);
        absPath[MAX_PATH_LEN - 1] = '\0';
    } else if (strncmp(path, "img/", 4) == 0) {

        if (strcmp(current_path, "/") == 0)
            snprintf(absPath, MAX_PATH_LEN, "/%s", path + 4);
        else
            snprintf(absPath, MAX_PATH_LEN, "%s/%s", current_path, path + 4);
    } else {

        if (strcmp(current_path, "/") == 0)
            snprintf(absPath, MAX_PATH_LEN, "/%s", path);
        else
            snprintf(absPath, MAX_PATH_LEN, "%s/%s", current_path, path);
    }
    printf("[DEBUG] create_directory: caminho absoluto: '%s'\n", absPath);
    
    // Se for raiz, nada a fazer.
    if (strcmp(absPath, "/") == 0)
        return 0;
    
    // Tokeniza o caminho e cria recursivamente os diretórios.
    char *pathCopy = strdup(absPath);
    if (!pathCopy) {
        printf("create_directory: erro ao alocar memória.\n");
        return -1;
    }
    
    char current[MAX_PATH_LEN] = "/";
    char *saveptr = NULL;
    char *token = strtok_r(pathCopy, "/", &saveptr);
    while (token != NULL) {
        char newPath[MAX_PATH_LEN];
        if (strcmp(current, "/") == 0)
            snprintf(newPath, sizeof(newPath), "/%s", token);
        else
            snprintf(newPath, sizeof(newPath), "%s/%s", current, token);
        
        printf("[DEBUG] create_directory: verificando '%s'\n", newPath);
        // Verifica se o diretório existe
        int cluster = find_directory_cluster(newPath);
        if (cluster < 0) {
            // Diretório não existe: cria-o
            // Salvamos o current_path e definimos para o diretório pai atual.
           
            char oldCurrent[MAX_PATH_LEN];
            strncpy(oldCurrent, current_path, sizeof(oldCurrent)-1);
            oldCurrent[sizeof(oldCurrent)-1] = '\0';
            strncpy(current_path, current, sizeof(current_path)-1);
            current_path[sizeof(current_path)-1] = '\0';
            
            printf("[DEBUG] create_directory: criando diretório '%s' em '%s'\n", token, current_path);
            cmd_mkdir(token);
            
            // Restaura current_path
            strncpy(current_path, oldCurrent, sizeof(current_path)-1);
            current_path[sizeof(current_path)-1] = '\0';
            
            // Verifica se foi criado
            cluster = find_directory_cluster(newPath);
            if (cluster < 0) {
                printf("create_directory: falha ao criar '%s'.\n", newPath);
                free(pathCopy);
                return -1;
            }
        }
        strcpy(current, newPath);
        token = strtok_r(NULL, "/", &saveptr);
    }
    
    free(pathCopy);
    return 0;
}

void format_fat_date_time(uint16_t date, uint16_t time, char *out, size_t out_size) {
    int day = date & 0x1F;
    int month = (date >> 5) & 0x0F;
    int year = ((date >> 9) & 0x7F) + 1980;
    int seconds = (time & 0x1F) * 2;
    int minutes = (time >> 5) & 0x3F;
    int hours = (time >> 11) & 0x1F;
    snprintf(out, out_size, "%04d-%02d-%02d %02d:%02d:%02d", year, month, day, hours, minutes, seconds);
}

int get_directory_entry(const char *path, DirectoryEntry *entry_out) {
    if (!path || !entry_out) {
        printf("get_directory_entry: parâmetros inválidos.\n");
        return -1;
    }
    
    // Cria uma cópia do caminho, pois dirname() e basename() podem modificá-la.
    char *pathCopy = strdup(path);
    if (!pathCopy) {
        printf("get_directory_entry: erro ao alocar memória para pathCopy.\n");
        return -1;
    }
    
    // Obtém o diretório pai e o nome base.
    // Note que dirname() e basename() podem usar o mesmo buffer (ou modificá-lo), então
    // fazemos cópias separadas para segurança.
    char *dirPathTemp = strdup(pathCopy);
    if (!dirPathTemp) {
        free(pathCopy);
        printf("get_directory_entry: erro ao alocar memória para dirPathTemp.\n");
        return -1;
    }
    char *baseTemp = strdup(pathCopy);
    if (!baseTemp) {
        free(pathCopy);
        free(dirPathTemp);
        printf("get_directory_entry: erro ao alocar memória para baseTemp.\n");
        return -1;
    }
    
    char *dirPathOrig = dirname(dirPathTemp);   // Pode modificar o buffer dirPathTemp.
    char *baseNameOrig = basename(baseTemp);      // Pode modificar o buffer baseTemp.
    
    // Se o diretório pai for ".", usamos current_path.
    char parentPath[MAX_PATH_LEN];
    if (strcmp(dirPathOrig, ".") == 0) {
        strncpy(parentPath, current_path, MAX_PATH_LEN - 1);
        parentPath[MAX_PATH_LEN - 1] = '\0';
    } else {
        strncpy(parentPath, dirPathOrig, MAX_PATH_LEN - 1);
        parentPath[MAX_PATH_LEN - 1] = '\0';
    }
    
    // Debug
    printf("[DEBUG] get_directory_entry: parentPath='%s', baseNameOrig='%s'\n", parentPath, baseNameOrig);
    
    // Localiza o cluster do diretório pai.
    int parentCluster = find_directory_cluster(parentPath);
    if (parentCluster < 0) {
        printf("get_directory_entry: diretório pai '%s' não encontrado.\n", parentPath);
        free(pathCopy);
        free(dirPathTemp);
        free(baseTemp);
        return -1;
    }
    
    // Calcula o tamanho do cluster
    size_t clusterSize = root->bootSector.bytesPerSector * root->bootSector.sectorsPerCluster;
    uint8_t *buffer = malloc(clusterSize);
    if (!buffer) {
        printf("get_directory_entry: erro ao alocar memória para buffer.\n");
        free(pathCopy);
        free(dirPathTemp);
        free(baseTemp);
        return -1;
    }
    
    if (fat32_read_cluster(root, parentCluster, buffer) != 0) {
        printf("get_directory_entry: erro ao ler o diretório '%s'.\n", parentPath);
        free(buffer);
        free(pathCopy);
        free(dirPathTemp);
        free(baseTemp);
        return -1;
    }
    
    // Converte o baseName para minúsculas para comparação
    char lowerBase[256];
    strncpy(lowerBase, baseNameOrig, sizeof(lowerBase)-1);
    lowerBase[sizeof(lowerBase)-1] = '\0';
    to_lowercase(lowerBase);
    
    // Percorre as entradas do diretório
    DirectoryEntry *entries = (DirectoryEntry *)buffer;
    int found = -1;
    for (size_t i = 0; i < clusterSize / sizeof(DirectoryEntry); i++) {
        // Se a entrada estiver vazia, finaliza a busca
        if ((unsigned char)entries[i].name[0] == 0x00)
            break;
        // Ignora entradas deletadas
        if ((unsigned char)entries[i].name[0] == 0xE5)
            continue;
        // Ignora entradas de LFN (atributo 0x0F)
        if ((entries[i].attributes & 0x0F) == 0x0F)
            continue;
        
        // Extrai o nome curto (8.3) e o converte para minúsculas
        char sfn[13] = {0};
        memcpy(sfn, entries[i].name, 11);
        sfn[11] = '\0';
        // Remover espaços à direita
        for (int j = 10; j >= 0 && sfn[j] == ' '; j--) {
            sfn[j] = '\0';
        }
        to_lowercase(sfn);
        
        // Debug: imprime a entrada encontrada
        // printf("[DEBUG] get_directory_entry: comparando '%s' com '%s'\n", sfn, lowerBase);
        
        if (strcmp(sfn, lowerBase) == 0) {
            memcpy(entry_out, &entries[i], sizeof(DirectoryEntry));
            found = 0;
            break;
        }
    }
    
    free(buffer);
    free(pathCopy);
    free(dirPathTemp);
    free(baseTemp);
    
    if (found < 0) {
        printf("get_directory_entry: entrada '%s' não encontrada no diretório '%s'.\n", lowerBase, parentPath);
        return -1;
    }
    return 0;
}

void to_lowercase(char *str) {
    if (!str) return;
    for (int i = 0; str[i] != '\0'; i++) {
        str[i] = tolower((unsigned char)str[i]);
    }
}

int find_directory_cluster(const char *path) {
    // Se for o diretório raiz, retorne o cluster raiz
    if (strcmp(path, "/") == 0) {
        return root->bootSector.rootCluster;
    }

    // Calcula o tamanho do cluster (usado para alocar buffer)
    size_t clusterSize = root->bootSector.bytesPerSector * root->bootSector.sectorsPerCluster;
    uint8_t *buffer = malloc(clusterSize);
    if (buffer == NULL) {
        printf("Erro ao alocar memória para leitura do diretório.\n");
        return -1;
    }

    // Cria uma cópia do caminho e tokeniza (removendo a barra inicial se existir)
    char *pathCopy = strdup(path);
    if (pathCopy == NULL) {
        free(buffer);
        printf("Erro ao duplicar o caminho.\n");
        return -1;
    }
    // Se o caminho começar com '/', pula-o
    char *relativePath = pathCopy;
    if (relativePath[0] == '/') {
        relativePath++;
    }

    // Tokeniza o caminho usando "/" como delimitador.
    char *token = strtok(relativePath, "/");

    // Inicia a busca a partir do cluster raiz
    int cluster = root->bootSector.rootCluster;

    while (token != NULL) {
        // Lê o cluster do diretório atual
        if (fat32_read_cluster(root, cluster, buffer) != 0) {
            printf("Erro ao ler o cluster do diretório.\n");
            free(buffer);
            free(pathCopy);
            return -1;
        }

        int found = 0;
        // Itera sobre as entradas do diretório
        size_t totalEntries = clusterSize / sizeof(DirectoryEntry);
        for (size_t i = 0; i < totalEntries; i++) {
            DirectoryEntry *entry = (DirectoryEntry *)&buffer[i * sizeof(DirectoryEntry)];

            // Se encontrar entrada vazia, encerra a busca nesse cluster.
            if ((unsigned char)entry->name[0] == 0x00) {
                break;
            }
            // Ignora entradas deletadas ou entradas de nome longo
            if ((unsigned char)entry->name[0] == 0xE5 || ((entry->attributes & 0x0F) == 0x0F)) {
                continue;
            }

            // Prepara o nome para comparação (primeiros 8 caracteres)
            char entryName[9];
            memcpy(entryName, entry->name, 8);
            entryName[8] = '\0';
            // Remove espaços à direita
            for (int j = 7; j >= 0 && entryName[j] == ' '; j--) {
                entryName[j] = '\0';
            }
            // Converte para minúsculas
            for (int j = 0; entryName[j]; j++) {
                entryName[j] = tolower(entryName[j]);
            }

            // Verifica se o nome coincide com o token procurado e se é um diretório.
            if (strcmp(entryName, token) == 0 && (entry->attributes & ATTR_DIRECTORY)) {
                // Calcula o número do cluster (FAT32: combina firstClusterHigh e firstClusterLow)
                cluster = (entry->firstClusterHigh << 16) | entry->firstClusterLow;
                found = 1;
                break;
            }
        }

        if (!found) {
            printf("Diretório '%s' não encontrado no caminho '%s'.\n", token, path);
            free(buffer);
            free(pathCopy);
            return -1;
        }

        token = strtok(NULL, "/");
    }

    free(buffer);
    free(pathCopy);
    return cluster;
}