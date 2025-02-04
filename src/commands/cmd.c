#include "cmd.h"
#include "../structs/fat32.h"

extern FAT32Partition *root;

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
    // TODO: Obtenha e exiba o caminho absoluto do diretório atual.
    printf("Exibindo o diretório corrente...\n");
}

// Exibe os atributos de um arquivo ou diretório.
void cmd_attr(const char *path) {
    // TODO: Leia e exiba os atributos do arquivo ou diretório (ex: tamanho, data de modificação).
    printf("Exibindo atributos de '%s'...\n", path);
}

// Altera o diretório corrente para o especificado.
void cmd_cd(const char *path) {
    // TODO: Navegue até o diretório especificado em 'path'.
    printf("Alterando o diretório corrente para '%s'...\n", path);
}

// Cria um arquivo vazio.
void cmd_touch(const char *file) {
    // TODO: Crie um novo arquivo vazio com o nome especificado em 'file'.
    printf("Criando arquivo vazio '%s'...\n", file);
}

// Cria um diretório vazio.
void cmd_mkdir(const char *dir) {
    // TODO: Crie um diretório vazio com o nome especificado em 'dir'.
    printf("Criando diretório '%s'...\n", dir);
}

// Remove o arquivo especificado.
void cmd_rm(const char *file) {
    // TODO: Remova o arquivo especificado por 'file'.
    printf("Removendo arquivo '%s'...\n", file);
}

// Remove o diretório especificado, se estiver vazio.
void cmd_rmdir(const char *dir) {
    // TODO: Remova o diretório especificado por 'dir', garantindo que esteja vazio.
    printf("Removendo diretório '%s'...\n", dir);
}

// Copia um arquivo de src para dst.
void cmd_cp(const char *src, const char *dst) {
    // TODO: Copie o arquivo de 'src' para 'dst'. Trate caminhos locais e da imagem (img/).
    printf("Copiando arquivo de '%s' para '%s'...\n", src, dst);
}

// Move um arquivo de src para dst.
void cmd_mv(const char *src, const char *dst) {
    // TODO: Mova o arquivo de 'src' para 'dst'. Trate caminhos locais e da imagem (img/).
    printf("Movendo arquivo de '%s' para '%s'...\n", src, dst);
}

// Renomeia um arquivo.
void cmd_rename(const char *file, const char *newname) {
    // TODO: Renomeie o arquivo especificado por 'file' para 'newname'.
    printf("Renomeando '%s' para '%s'...\n", file, newname);
}

// Lista os arquivos e diretórios do diretório corrente.
void cmd_ls() {
    // TODO: Leia e exiba os arquivos e diretórios do diretório atual.
    printf("Listando arquivos e diretórios...\n");
}

