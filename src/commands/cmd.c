/*  
===============================================================================  
Nome do Projeto : FATal32 
Descrição       : Esse é o código cmd.c ele faz as operações utilizando as funções 
                  e estruturas disponiblizadas na /structs/fat32.h e funções da /utils/utils.h.
                  Aqui cada comando está em snake case acompanhado do prefixo cmd_, por exemplo o
                  comando ls, seu nome de função é cmd_ls.                      
Autor           : Matheus V. Costa  
Data de Criação : 25/12/2024  
Última Alteração: 14/02/2024  
===============================================================================  
*/

#include "cmd.h"
#include "../structs/fat32.h"
#include "../shell/shell.h"
#include "../utils/utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

extern FAT32Partition *root;
extern char current_path[];

// Exibe informações do disco e da FAT.
void cmd_info() {
    fat32_print_info(root);
}

// Exibe o conteúdo do bloco especificado no formato texto.
void cmd_cluster(int num) {
    size_t clusterSize = root->bootSector.bytesPerSector * root->bootSector.sectorsPerCluster;
    uint8_t *buffer = malloc(clusterSize);
    if (buffer == NULL) {
        printf("Erro ao alocar memória para o cluster %d.\n", num);
        return;
    }
    if (fat32_read_cluster(root, num, buffer) == 0) {
        printf("Conteúdo do cluster %d:\n", num);
        for (size_t i = 0; i < clusterSize; ++i) {
            printf("%c", (buffer[i] >= 32 && buffer[i] <= 126) ? buffer[i] : '.');
        }
        printf("\n");
    } else {
        printf("Erro ao ler o cluster %d.\n", num);
    }
    free(buffer);
}

// Exibe o diretório corrente (caminho absoluto).
void cmd_pwd() {
    printf("%s\n", current_path);
}

// Comando: Exibe os atributos de um arquivo ou diretório.
void cmd_attr(const char *path) {
    printf("Exibindo atributos de '%s'...\n", path);
    
    // Converte o caminho para um formato absoluto interno
    char absPath[MAX_PATH_LEN];
    if (path[0] == '/') {
        strncpy(absPath, path, MAX_PATH_LEN - 1);
        absPath[MAX_PATH_LEN - 1] = '\0';
    } else if (strncmp(path, "img/", 4) == 0) {
        // Se o usuário usa o prefixo "img/", removemos-o
        snprintf(absPath, MAX_PATH_LEN, "/%s", path + 4);
    } else {
  
        if (strcmp(current_path, "/") == 0)
            snprintf(absPath, MAX_PATH_LEN, "/%s", path);
        else
            snprintf(absPath, MAX_PATH_LEN, "%s/%s", current_path, path);
    }
    printf("[DEBUG] cmd_attr: caminho absoluto: '%s'\n", absPath);
    
    DirectoryEntry entry;
    if (get_directory_entry(absPath, &entry) != 0) {
        printf("Erro: Entrada para '%s' não encontrada.\n", absPath);
        return;
    }
    
    // Exibe atributos básicos
    printf("Nome curto: ");
    for (int i = 0; i < 11; i++) {
        if (entry.name[i] == ' ')
            break;
        putchar(entry.name[i]);
    }
    putchar('\n');
    
    // Atributos (exemplo: diretório, arquivo, somente leitura, etc.)
    printf("Tipo: %s\n", (entry.attributes & ATTR_DIRECTORY) ? "Diretório" : "Arquivo");
    printf("Atributos brutos: 0x%02X\n", entry.attributes);
    
    // Tamanho (apenas arquivos têm tamanho diferente de 0 normalmente)
    printf("Tamanho: %u bytes\n", entry.fileSize);
    
    // Datas – criação, última modificação, último acesso.
    char creationStr[32], modificationStr[32], accessStr[32];
    format_fat_date_time(entry.creationDate, entry.creationTime, creationStr, sizeof(creationStr));
    format_fat_date_time(entry.writeDate, entry.writeTime, modificationStr, sizeof(modificationStr));
    // Para a data de último acesso, só temos a data (sem hora)
    {
        int day = entry.lastAccessDate & 0x1F;
        int month = (entry.lastAccessDate >> 5) & 0x0F;
        int year = ((entry.lastAccessDate >> 9) & 0x7F) + 1980;
        snprintf(accessStr, sizeof(accessStr), "%04d-%02d-%02d", year, month, day);
    }
    
    printf("Criado: %s\n", creationStr);
    printf("Modificado: %s\n", modificationStr);
    printf("Último acesso: %s\n", accessStr);
    
    // Cluster de início
    uint32_t startCluster = (entry.firstClusterHigh << 16) | entry.firstClusterLow;
    printf("Cluster inicial: %u\n", startCluster);
}


// Altera o diretório corrente para o especificado.
void cmd_cd(const char *path) {
    // Tratar caminho absoluto (começa com '/')
    if (path[0] == '/') {
        strcpy(current_path, "/");
        if (strlen(path) > 1) {
            cmd_cd(path + 1); // Processa o restante do caminho
        }
        return;
    }

    // Verificar se o caminho é para voltar diretórios
    if (strncmp(path, "..", 2) == 0) {
        int levels = 0;
        const char *p = path;
        while (strncmp(p, "..", 2) == 0) {
            levels++;
            p += 2;
            if (*p == '/') p++;
        }

        for (int i = 0; i < levels; i++) {
            char *last_slash = strrchr(current_path, '/');
            if (last_slash != NULL) {
                if (last_slash == current_path) {
                    // Não pode voltar além da raiz
                    current_path[1] = '\0';
                } else {
                    *last_slash = '\0';
                }
            }
        }

        // Se ainda houver algo no path após os '..', precisa processsar
        if (*p != '\0') {
            cmd_cd(p);
        }
        return;
    }

    size_t clusterSize = root->bootSector.bytesPerSector * root->bootSector.sectorsPerCluster;
    uint8_t *buffer = malloc(clusterSize);
    if (buffer == NULL) {
        printf("ERRO FATAL: Falha ao alocar memória.\n");
        return;
    }

    // Achar o cluster do diretório atual, talvez fazer isso no FAT32.c
    int current_cluster = root->bootSector.rootCluster;
    if (strcmp(current_path, "/") != 0) {
        char *path_copy = strdup(current_path);
        if (path_copy == NULL) {
            printf("ERRO FATAL: Falha ao alocar memória.\n");
            free(buffer);
            return;
        }

        char *component = strtok(path_copy + 1, "/");
        while (component != NULL) {
            uint8_t *dir_buffer = malloc(clusterSize);
            if (dir_buffer == NULL) {
                printf("ERRO FATAL: Falha ao alocar memória.\n");
                free(path_copy);
                free(buffer);
                return;
            }

            if (fat32_read_cluster(root, current_cluster, dir_buffer) != 0) {
                printf("ERRO FATAL: Falha ao ler cluster %d.\n", current_cluster);
                free(dir_buffer);
                free(path_copy);
                free(buffer);
                return;
            }

            DirectoryEntry *entry = (DirectoryEntry *)dir_buffer;
            int found = 0;

            for (size_t i = 0; i < clusterSize / sizeof(DirectoryEntry); ++i) {
                if (entry[i].name[0] == 0x00) break;
                if ((unsigned char)entry[i].name[0] == 0xE5) continue;
                if ((entry[i].attributes & 0x0F) == 0x0F) continue;

                char name[9];
                memcpy(name, entry[i].name, 8);
                name[8] = '\0';
                for (int j = 7; j >= 0 && name[j] == ' '; j--) name[j] = '\0';
                for (int j = 0; name[j]; j++) name[j] = tolower(name[j]);

                char lower_component[256];
                strncpy(lower_component, component, 255);
                for (int j = 0; lower_component[j]; j++) lower_component[j] = tolower(lower_component[j]);

                if (strcmp(name, lower_component) == 0 && (entry[i].attributes & 0x10)) {
                    current_cluster = (entry[i].firstClusterHigh << 16) | entry[i].firstClusterLow;
                    found = 1;
                    break;
                }
            }

            free(dir_buffer);
            if (!found) {
                printf("ERRO FATAL: Diretório '%s' não encontrado.\n", component);
                free(path_copy);
                free(buffer);
                return;
            }
            component = strtok(NULL, "/");
        }
        free(path_copy);
    }

    // Ler o diretório atual
    if (fat32_read_cluster(root, current_cluster, buffer) != 0) {
        printf("ERRO FATAL: Falha ao ler cluster %d.\n", current_cluster);
        free(buffer);
        return;
    }

    // Procurar pelo diretório especificado
    DirectoryEntry *entry = (DirectoryEntry *)buffer;
    int found = 0;
    char target_name[9];

    for (size_t i = 0; i < clusterSize / sizeof(DirectoryEntry); ++i) {
        if (entry[i].name[0] == 0x00) break;
        if ((unsigned char)entry[i].name[0] == 0xE5) continue;
        if ((entry[i].attributes & 0x0F) == 0x0F) continue;

        memcpy(target_name, entry[i].name, 8);
        target_name[8] = '\0';
        for (int j = 7; j >= 0 && target_name[j] == ' '; j--) target_name[j] = '\0';
        for (int j = 0; target_name[j]; j++) target_name[j] = tolower(target_name[j]);

        char lower_path[256];
        strncpy(lower_path, path, 255);
        for (int j = 0; lower_path[j]; j++) lower_path[j] = tolower(lower_path[j]);

        if (strcmp(target_name, lower_path) == 0 && (entry[i].attributes & 0x10)) {
            found = 1;
            break;
        }
    }

    if (found) {
        char new_path[256];
        if (strcmp(current_path, "/") == 0) {
            snprintf(new_path, sizeof(new_path), "/%s", path);
        } else {
            snprintf(new_path, sizeof(new_path), "%s/%s", current_path, path);
        }
        strncpy(current_path, new_path, sizeof(current_path) - 1);
        current_path[sizeof(current_path) - 1] = '\0';
    } else {
        printf("ERRO: Diretório '%s' não encontrado.\n", path);
    }

    free(buffer);
}

// Cria um arquivo vazio.
void cmd_touch(const char *file) {
    printf("Criando arquivo vazio '%s'...\n", file);

    // Determinar o cluster do diretório atual
    int parentCluster = 0;
    if (strcmp(current_path, "/") != 0) {
        parentCluster = find_directory_cluster(current_path);
        if (parentCluster < 0) {
            printf("Erro: diretório atual não localizado.\n");
            return;
        }
    } else {
        parentCluster = root->bootSector.rootCluster;
    }

    // Lê o cluster do diretório atual.
    size_t clusterSize = root->bootSector.bytesPerSector * root->bootSector.sectorsPerCluster;
    uint8_t *parentBuffer = malloc(clusterSize);
    if (parentBuffer == NULL) {
        printf("Erro: sem memória para ler o diretório atual.\n");
        return;
    }
    if (fat32_read_cluster(root, parentCluster, parentBuffer) != 0) {
        printf("Erro ao ler o cluster do diretório atual.\n");
        free(parentBuffer);
        return;
    }

    // Procura entrada livre no dir (0x00 - 0xE5).
    int freeIndex = -1;
    size_t totalEntries = clusterSize / sizeof(DirectoryEntry);
    for (size_t i = 0; i < totalEntries; i++) {
        DirectoryEntry *entry = (DirectoryEntry *)&parentBuffer[i * sizeof(DirectoryEntry)];
        if ((unsigned char)entry->name[0] == 0x00 || (unsigned char)entry->name[0] == 0xE5) {
            freeIndex = i;
            break;
        }
    }
    if (freeIndex == -1) {
        printf("Erro: Não há espaço livre no diretório atual.\n");
        free(parentBuffer);
        return;
    }

    // Nova entrada para o arquivo.
    DirectoryEntry newEntry;
    memset(&newEntry, 0, sizeof(DirectoryEntry));

    // Converte o nome informado para o formato curto (8.3 – 11 bytes)
    char newShort[11];
    format_short_name(file, newShort);
    memcpy(newEntry.name, newShort, 11);

    // Define os atributos: 0x20 para arquivo (bit archive)
    newEntry.attributes = 0x20;

    // Arquivo vazio: tamanho 0 e sem cluster alocado (campos de cluster em 0)
    newEntry.firstClusterHigh = 0;
    newEntry.firstClusterLow  = 0;
    newEntry.fileSize = 0;

    DirectoryEntry *entryPtr = (DirectoryEntry *)&parentBuffer[freeIndex * sizeof(DirectoryEntry)];
    memcpy(entryPtr, &newEntry, sizeof(DirectoryEntry));

    // Escrita no cluster 
    if (fat32_write_cluster(root, parentCluster, parentBuffer) != 0) {
        printf("Erro ao escrever alterações no diretório atual.\n");
        free(parentBuffer);
        return;
    }

    

    free(parentBuffer);
    printf("Arquivo '%s' criado com sucesso.\n", file);
}

// Cria um diretório vazio.
void cmd_mkdir(const char *dir) {
    printf("Criando diretório '%s'...\n", dir);

    // Determina o cluster do diretório atual (diretório pai onde será criado o novo diretório).
    int parentCluster = 0;
    if (strcmp(current_path, "/") != 0) {
        parentCluster = find_directory_cluster(current_path);
        if (parentCluster < 0) {
            printf("Erro: diretório atual não localizado.\n");
            return;
        }
    } else {
        parentCluster = root->bootSector.rootCluster;
    }

    // Lê o cluster do diretório pai para inserir a nova entrada.
    size_t clusterSize = root->bootSector.bytesPerSector * root->bootSector.sectorsPerCluster;
    uint8_t *parentBuffer = malloc(clusterSize);
    if (parentBuffer == NULL) {
        printf("Erro: sem memória para ler o diretório pai.\n");
        return;
    }
    if (fat32_read_cluster(root, parentCluster, parentBuffer) != 0) {
        printf("Erro ao ler o cluster do diretório pai.\n");
        free(parentBuffer);
        return;
    }

    // Procura uma entrada livre no diretório pai.
    int freeIndex = -1;
    size_t totalEntries = clusterSize / sizeof(DirectoryEntry);
    for (size_t i = 0; i < totalEntries; i++) {
        DirectoryEntry *entry = (DirectoryEntry *)&parentBuffer[i * sizeof(DirectoryEntry)];
        if ((unsigned char)entry->name[0] == 0x00 || (unsigned char)entry->name[0] == 0xE5) {
            freeIndex = i;
            break;
        }
    }
    if (freeIndex == -1) {
        printf("Erro: Não há entradas livres no diretório pai.\n");
        free(parentBuffer);
        return;
    }

    // Aloca um novo cluster para o novo diretório.
    int newDirCluster = fat32_allocate_cluster(root);
    if (newDirCluster < 0) {
        printf("Erro: Falha ao alocar um novo cluster para o diretório.\n");
        free(parentBuffer);
        return;
    }

    // Preenche a entrada do diretório pai para o novo diretório.
    DirectoryEntry *newEntry = (DirectoryEntry *)&parentBuffer[freeIndex * sizeof(DirectoryEntry)];
    char newShort[11];
    format_short_name(dir, newShort);
    memcpy(newEntry->name, newShort, 11);
    newEntry->attributes = ATTR_DIRECTORY;
 
    newEntry->firstClusterHigh = (uint16_t)(newDirCluster >> 16);
    newEntry->firstClusterLow  = (uint16_t)(newDirCluster & 0xFFFF);
    newEntry->fileSize = 0; // tamanho 0

    // Escreve o cluster modificado do diretório pai na imagem.
    if (fat32_write_cluster(root, parentCluster, parentBuffer) != 0) {
        printf("Erro ao escrever alterações no diretório pai.\n");
        free(parentBuffer);
        return;
    }
    free(parentBuffer);

    // Inicializa o cluster do novo diretório.
    uint8_t *newDirBuffer = malloc(clusterSize);
    if (newDirBuffer == NULL) {
        printf("Erro: sem memória para inicializar o novo diretório.\n");
        return;
    }
    memset(newDirBuffer, 0, clusterSize);

    // Cria a entrada "." que aponta para o próprio diretório.
    DirectoryEntry dotEntry;
    memset(&dotEntry, 0, sizeof(DirectoryEntry));
    // O nome curto para "." e depois dois espaço "  "
    memcpy(dotEntry.name, ".          ", 11);
    dotEntry.attributes = ATTR_DIRECTORY;
    dotEntry.firstClusterHigh = (uint16_t)(newDirCluster >> 16);
    dotEntry.firstClusterLow  = (uint16_t)(newDirCluster & 0xFFFF);
    dotEntry.fileSize = 0;

    // Cria a entrada ".." que é o diretório pai.
    DirectoryEntry dotdotEntry;
    memset(&dotdotEntry, 0, sizeof(DirectoryEntry));
    memcpy(dotdotEntry.name, "..         ", 11);
    dotdotEntry.attributes = ATTR_DIRECTORY;
    dotdotEntry.firstClusterHigh = (uint16_t)(parentCluster >> 16);
    dotdotEntry.firstClusterLow  = (uint16_t)(parentCluster & 0xFFFF);
    dotdotEntry.fileSize = 0;

    // Copia as entradas "." e ".." para o início do novo diretório.
    memcpy(newDirBuffer, &dotEntry, sizeof(DirectoryEntry));
    memcpy(newDirBuffer + sizeof(DirectoryEntry), &dotdotEntry, sizeof(DirectoryEntry));

    if (fat32_write_cluster(root, newDirCluster, newDirBuffer) != 0) {
        printf("Erro ao escrever o novo diretório.\n");
        free(newDirBuffer);
        return;
    }
    free(newDirBuffer);

    printf("Diretório '%s' criado com sucesso.\n", dir);
}

// Remove o arquivo especificado.
void cmd_rm(const char *file) {
    printf("Removendo arquivo '%s'...\n", file);

    // Determina o cluster do diretório atual.
    int cluster = 0;
    if (strcmp(current_path, "/") != 0) {
        cluster = find_directory_cluster(current_path);
        if (cluster < 0) {
            printf("Erro: diretório atual não localizado.\n");
            return;
        }
    } else {
        cluster = root->bootSector.rootCluster;
    }

    // Lê o cluster do diretório atual.
    size_t clusterSize = root->bootSector.bytesPerSector * root->bootSector.sectorsPerCluster;
    uint8_t *buffer = malloc(clusterSize);
    if (buffer == NULL) {
        printf("Erro: sem memória para ler o diretório.\n");
        return;
    }
    if (fat32_read_cluster(root, cluster, buffer) != 0) {
        printf("Erro ao ler o cluster do diretório.\n");
        free(buffer);
        return;
    }

    // Procura a entrada do arquivo no diretório.
    DirectoryEntry *entry = (DirectoryEntry *)buffer;
    int found = 0;
    int entryIndex = -1;
    size_t totalEntries = clusterSize / sizeof(DirectoryEntry);

    for (size_t i = 0; i < totalEntries; i++) {
        if (entry[i].name[0] == 0x00) break; // Fim das entradas
        if ((unsigned char)entry[i].name[0] == 0xE5) continue; // Entrada deletada
        if ((entry[i].attributes & 0x0F) == 0x0F) continue; // Entrada de nome longo

        // Extrai o nome curto (8.3) da entrada.
        char name[9];
        memcpy(name, entry[i].name, 8);
        name[8] = '\0';
        for (int j = 7; j >= 0 && name[j] == ' '; j--) name[j] = '\0';

        // Converte o nome para minúsculas.
        for (int j = 0; name[j]; j++) name[j] = tolower(name[j]);

        // Converte o nome do arquivo para minúsculas.
        char lower_file[256];
        strncpy(lower_file, file, 255);
        for (int j = 0; lower_file[j]; j++) lower_file[j] = tolower(lower_file[j]);

        // Compara o nome da entrada com o arquivo procurado.
        if (strcmp(name, lower_file) == 0 && !(entry[i].attributes & 0x10)) {
            found = 1;
            entryIndex = i;
            break;
        }
    }

    if (!found) {
        printf("Erro: Arquivo '%s' não encontrado.\n", file);
        free(buffer);
        return;
    }

    // Marca a entrada como deletada (0xE5 no primeiro byte do nome).
    entry[entryIndex].name[0] = 0xE5;

    // Libera os clusters associados ao arquivo na FAT.
    uint16_t firstClusterHigh = entry[entryIndex].firstClusterHigh;
    uint16_t firstClusterLow = entry[entryIndex].firstClusterLow;
    uint32_t clusterNumber = (firstClusterHigh << 16) | firstClusterLow;

    

    if (clusterNumber != 0) {
        if (fat32_free_cluster_chain(root, clusterNumber) != 0) {
            printf("Erro ao liberar clusters na FAT.\n");
            free(buffer);
            return;
        }
    }

    // Escreve o cluster modificado de volta no disco.
    if (fat32_write_cluster(root, cluster, buffer) != 0) {
        printf("Erro ao escrever alterações no diretório.\n");
        free(buffer);
        return;
    }

    free(buffer);
    printf("Arquivo '%s' removido com sucesso.\n", file);
}

// Remove o diretório especificado, se estiver vazio.
void cmd_rmdir(const char *dir) {
    printf("Removendo diretório '%s'...\n", dir);

    // Encontrar o cluster do diretório atual
    int parentCluster = find_directory_cluster(current_path);
    if (parentCluster < 0) {
        printf("Erro: Diretório atual não encontrado.\n");
        return;
    }

    size_t clusterSize = root->bootSector.bytesPerSector * root->bootSector.sectorsPerCluster;
    uint8_t *buffer = malloc(clusterSize);
    if (!buffer) {
        printf("Erro ao alocar memória.\n");
        return;
    }

    if (fat32_read_cluster(root, parentCluster, buffer) != 0) {
        printf("Erro ao ler o diretório pai.\n");
        free(buffer);
        return;
    }

    DirectoryEntry *entry = (DirectoryEntry *)buffer;
    int found = 0;
    int entryIndex = -1;
    uint32_t dirCluster = 0;

    for (size_t i = 0; i < clusterSize / sizeof(DirectoryEntry); i++) {
        if (entry[i].name[0] == 0x00) break;
        if ((unsigned char)entry[i].name[0] == 0xE5) continue;
        if ((entry[i].attributes & 0x0F) == 0x0F) continue; // Ignorar LFN entries

        char name[13];
        memcpy(name, entry[i].name, 11);
        name[11] = '\0';
        for (int j = 10; j >= 0 && name[j] == ' '; j--) name[j] = '\0'; // Remover espaços extras

        if (strcasecmp(name, dir) == 0 && (entry[i].attributes & 0x10)) {
            found = 1;
            entryIndex = i;
            dirCluster = (entry[i].firstClusterHigh << 16) | entry[i].firstClusterLow;
            break;
        }
    }

    if (!found) {
        printf("Erro: Diretório '%s' não encontrado.\n", dir);
        free(buffer);
        return;
    }

    // Verificar se o diretório está vazio
    if (!is_directory_empty(root, dirCluster)) {
        printf("AVISO: O diretório não está vazio. Deseja excluir recursivamente? (S/N): ");
        char resposta;
        scanf(" %c", &resposta);
        if (tolower(resposta) != 's') {
            printf("Operação cancelada.\n");
            free(buffer);
            return;
        }

        // Construir o caminho base para `recursive_delete_dir`
        char basePath[512];
        snprintf(basePath, sizeof(basePath), "%s/%s", current_path, dir);

        recursive_delete_dir(root, dirCluster, basePath);
    }

    // Marcar entrada como deletada e atualizar a FAT
    entry[entryIndex].name[0] = 0xE5;
    if (fat32_write_cluster(root, parentCluster, buffer) != 0) {
        printf("Erro ao atualizar diretório pai.\n");
        free(buffer);
        return;
    }

    fat32_free_cluster_chain(root, dirCluster);

    free(buffer);
    printf("Diretório '%s' removido com sucesso.\n", dir);
}

// Copia um arquivo de src para dst.
void cmd_cp(const char *src, const char *dst) {
    printf("Copiando de '%s' para '%s'...\n", src, dst);

    // Helper
    int is_internal(const char *path) {
        return strncmp(path, "img/", 4) == 0;
    }

    // Helper
    char* get_absolute_image_path(const char *path) {
        char *absPath = malloc(MAX_PATH_LEN);
        if (!absPath) return NULL;
        
        if (strncmp(path, "img/", 4) == 0) {
            snprintf(absPath, MAX_PATH_LEN, "%s", path + 4);
        } else {
            snprintf(absPath, MAX_PATH_LEN, "%s/%s", current_path, path);
        }
        normalize_path(absPath); // Remove barras duplicadas
        return absPath;
    }

    // Cópia interna → interna (img/path1 → img/path2)
    if (is_internal(src) && is_internal(dst)) {
        char *srcPath = get_absolute_image_path(src);
        char *dstPath = get_absolute_image_path(dst);
        
        // Verifica se destino é diretório
        int dstIsDir = is_directory(root, dstPath);
        if (dstIsDir) {
            // Adiciona o nome do arquivo ao destino
            char *fileName = get_filename(srcPath);
            char newDstPath[MAX_PATH_LEN];
            snprintf(newDstPath, sizeof(newDstPath), "%s/%s", dstPath, fileName);
            free(dstPath);
            dstPath = strdup(newDstPath);
        }


            //PAREI AQUI!!!!!!!!!!!!!!!!!!!!!!!!!


        // Lê arquivo de origem
        uint8_t *data = NULL;
        size_t size = 0;
        if (read_internal_file(srcPath, &data, &size) != 0) {
            printf("Erro ao ler arquivo de origem!\n");
            goto cleanup;
        }

        // Escreve arquivo de destino
        if (write_internal_file(dstPath, data, size) != 0) {
            printf("Erro ao escrever arquivo de destino!\n");
        }

        cleanup:
            free(srcPath);
            free(dstPath);
            free(data);
    }
    // Cópia externa → interna (sistema → img/path)
    else if (!is_internal(src) && is_internal(dst)) {
        char *dstPath = get_absolute_image_path(dst);
        
        // Lê arquivo externo
        FILE *file = fopen(src, "rb");
        if (!file) {
            printf("Erro ao abrir arquivo externo!\n");
            free(dstPath);
            return;
        }
        
        fseek(file, 0, SEEK_END);
        size_t size = ftell(file);
        fseek(file, 0, SEEK_SET);
        
        uint8_t *data = malloc(size);
        if (!data) {
            printf("Erro de memória!\n");
            fclose(file);
            free(dstPath);
            return;
        }
        
        fread(data, 1, size, file);
        fclose(file);

        // Escreve na imagem
        if (write_internal_file(dstPath, data, size) != 0) {
            printf("Erro ao escrever na imagem!\n");
        }

        free(dstPath);
        free(data);
    }
    //Cópia interna → externa (img/path → sistema)
    else if (is_internal(src) && !is_internal(dst)) {
        char *srcPath = get_absolute_image_path(src);
        
        // Lê arquivo interno
        uint8_t *data = NULL;
        size_t size = 0;
        if (read_internal_file(srcPath, &data, &size) != 0) {
            printf("Erro ao ler arquivo da imagem!\n");
            free(srcPath);
            return;
        }

        // Escreve arquivo externo
        FILE *file = fopen(dst, "wb");
        if (!file) {
            printf("Erro ao criar arquivo externo!\n");
            free(srcPath);
            free(data);
            return;
        }
        
        fwrite(data, 1, size, file);
        fclose(file);

        free(srcPath);
        free(data);
    }
    else {
        printf("Combinação de caminhos inválida!\n");
    }
}

// Move um arquivo de src para dst.
void cmd_mv(const char *src, const char *dst) {
    printf("Função não implementada...\n", src, dst);
    //Fazer depois de arrumar o cmd_cp
}

/// Função que renomeia um arquivo ou diretório.
void cmd_rename(const char *file, const char *newname) {
    printf("Renomeando '%s' para '%s'...\n", file, newname);

    //cluster do diretório atual.
    int cluster;
    if (strcmp(current_path, "/") != 0) {
        cluster = find_directory_cluster(current_path);
        if (cluster < 0) {
            printf("Erro: diretório atual não localizado.\n");
            return;
        }
    } else {
        cluster = root->bootSector.rootCluster;
    }

    //Lê o cluster do diretório atual.
    size_t clusterSize = root->bootSector.bytesPerSector * root->bootSector.sectorsPerCluster;
    uint8_t *buffer = malloc(clusterSize);
    if (!buffer) {
        printf("Erro ao alocar memória para leitura do diretório.\n");
        return;
    }
    if (fat32_read_cluster(root, cluster, buffer) != 0) {
        printf("Erro ao ler o cluster do diretório.\n");
        free(buffer);
        return;
    }

    //Procura a entrada cuja versão curta corresponda a 'file'.
    int found = 0;
    int mainIndex = -1;
    size_t totalEntries = clusterSize / sizeof(DirectoryEntry);
    for (size_t i = 0; i < totalEntries; i++) {
        DirectoryEntry *entry = (DirectoryEntry *)&buffer[i * sizeof(DirectoryEntry)];

        if ((unsigned char)entry->name[0] == 0x00)  // fim das entradas
            break;
        if ((unsigned char)entry->name[0] == 0xE5)  // entrada deletada
            continue;
        // Ignora entradas LFN (atributo 0x0F)
        if ((entry->attributes & 0x0F) == 0x0F)
            continue;

        char shortName[13] = {0};
        memcpy(shortName, entry->name, 11);
        shortName[11] = '\0';
        // Remove espaços à direita.
        for (int j = 10; j >= 0; j--) {
            if (shortName[j] == ' ')
                shortName[j] = '\0';
            else
                break;
        }
        char cmpName[256];
        strncpy(cmpName, shortName, sizeof(cmpName) - 1);
        cmpName[sizeof(cmpName) - 1] = '\0';
        str_to_upper(cmpName);
        char fileUpper[256];
        strncpy(fileUpper, file, sizeof(fileUpper) - 1);
        fileUpper[sizeof(fileUpper) - 1] = '\0';
        str_to_upper(fileUpper);

        if (strcmp(cmpName, fileUpper) == 0) {
            found = 1;
            mainIndex = i;
            break;
        }
    }

    if (!found) {
        printf("Arquivo ou diretório '%s' não encontrado.\n", file);
        free(buffer);
        return;
    }

    //Atualiza as entradas LFN (caso existam)
    int updatedLFN = update_lfn_entries(buffer, mainIndex, newname);
    if (updatedLFN < 0) {
        free(buffer);
        return;
    }

    //Atualiza a entrada curta com o novo nome formatado.
    char newShort[11];
    format_short_name(newname, newShort);
    DirectoryEntry *entry = (DirectoryEntry *)&buffer[mainIndex * sizeof(DirectoryEntry)];
    memcpy(entry->name, newShort, 11);

    //Recalcula o checksum e atualiza-o em todas as entradas LFN atualizadas.
    uint8_t sum = calc_lfn_checksum(newShort);
    for (int i = mainIndex - updatedLFN; i < mainIndex; i++) {
        LFNEntry *lfn = (LFNEntry *)&buffer[i * sizeof(DirectoryEntry)];
        lfn->checksum = sum;
    }

    //Escreve o cluster modificado de volta na imagem.
    if (fat32_write_cluster(root, cluster, buffer) != 0) {
        printf("Erro ao escrever o cluster do diretório.\n");
        free(buffer);
        return;
    }

    free(buffer);
    printf("Arquivo/diretório renomeado com sucesso.\n");
}

// Lista os arquivos e diretórios do diretório corrente.
void cmd_ls() {
    size_t clusterSize = root->bootSector.bytesPerSector * root->bootSector.sectorsPerCluster;
    uint8_t *buffer = malloc(clusterSize);
    if (buffer == NULL) {
        printf("Erro ao alocar memória para leitura do diretório.\n");
        return;
    }

    // Determina o cluster do diretório atual.
    int cluster = 0;
    if (strcmp(current_path, "/") != 0) {
        cluster = find_directory_cluster(current_path);
        if (cluster < 0) {
            free(buffer);
            return;
        }
    } else {
        cluster = root->bootSector.rootCluster;
    }

    if (fat32_read_cluster(root, cluster, buffer) != 0) {
        printf("Erro ao ler o cluster do diretório.\n");
        free(buffer);
        return;
    }

    size_t totalEntries = clusterSize / sizeof(DirectoryEntry);
    // Buffer para armazenar nome longo (até 64 caracteres)
    char longName[64] = {0};
    int acumulandoLFN = 0; // flag indicando se estamos acumulando entradas LFN

    for (size_t i = 0; i < totalEntries; i++) {
        DirectoryEntry *entry = (DirectoryEntry *)&buffer[i * sizeof(DirectoryEntry)];

        // Se a entrada estiver vazia, finaliza a iteração
        if ((unsigned char)entry->name[0] == 0x00) {
            break;
        }
        // Se for entrada deletada, pula
        if ((unsigned char)entry->name[0] == 0xE5) {
            continue;
        }

        // Verifica se é entrada LFN (atributo 0x0F)
        if ((entry->attributes & 0x0F) == 0x0F) {
            // Trata como LFN: cast para LFNEntry e acumula a parte do nome
            LFNEntry *lfn = (LFNEntry *)entry;
            // Se for a primeira entrada LFN para este arquivo, zera o buffer
            if (!acumulandoLFN) {
                memset(longName, 0, sizeof(longName));
                acumulandoLFN = 1;
            }
            // Cria um buffer temporário para os caracteres desta entrada
            char parte[14] = {0};  // máximo 13 caracteres (5+6+2)
            lfn_extract_part(lfn, parte, sizeof(parte));
            size_t lenParte = strlen(parte);
            size_t lenLongName = strlen(longName);
            if (lenParte + lenLongName < sizeof(longName)) {
                // Move o que já estava no longName para abrir espaço no início
                memmove(longName + lenParte, longName, lenLongName + 1);
                memcpy(longName, parte, lenParte);
            }
            continue;
        } else {

            if (acumulandoLFN) {
                // Converte para minúsculas
                for (int j = 0; longName[j]; j++) {
                    longName[j] = tolower(longName[j]);
                }
                printf("%s\n", longName);
                // Reset de buffer
                memset(longName, 0, sizeof(longName));
                acumulandoLFN = 0;
            } else {
                // Não tem LFN, use entrada normal (8.3)
                char name[9] = {0};
                char ext[4] = {0};
                memcpy(name, entry->name, 8);
                name[8] = '\0';
                memcpy(ext, entry->name + 8, 3);
                ext[3] = '\0';
                // Remove espaços à direita
                for (int j = 7; j >= 0 && name[j] == ' '; j--) {
                    name[j] = '\0';
                }
                for (int j = 2; j >= 0 && ext[j] == ' '; j--) {
                    ext[j] = '\0';
                }
                // Converte para minúsculas
                for (int j = 0; name[j]; j++) {
                    name[j] = tolower(name[j]);
                }
                for (int j = 0; ext[j]; j++) {
                    ext[j] = tolower(ext[j]);
                }
                if (ext[0] != '\0') {
                    printf("%s.%s\n", name, ext);
                } else {
                    printf("%s\n", name);
                }
            }
        }
    }

    free(buffer);
}