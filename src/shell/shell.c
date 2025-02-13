#include "shell.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

// Definir tamanho máximo de comando
#define MAX_COMMAND_SIZE 256

// Variável global para armazenar a partição FAT32 montada
FAT32Partition *root = NULL;
char absolute_image_path[256];
char program_path[256];
char current_path[256];


void start_shell(const char *image_path) {
    system("clear");
    print_ascii_art();

    // Armazena o caminho absoluto da imagem
    realpath(image_path, absolute_image_path);

    // Armazena o caminho do programa
    if (getcwd(program_path, sizeof(program_path)) == NULL) {
        fprintf(stderr, "ERRO FATAL: Falha ao obter o caminho do programa.\n");
        exit(EXIT_FAILURE);
    }

    // Monta a imagem FAT32
    root = malloc(sizeof(FAT32Partition));
    if (root == NULL) {
        fprintf(stderr, "ERRO FATAL: Falha ao alocar memória para a partição FAT32.\n");
        exit(EXIT_FAILURE);
    }

    if (mount_fat32(image_path, root) != 0) {
        fprintf(stderr, "ERRO FATAL: Falha ao montar a imagem FAT32.\n");
        free(root);
        exit(EXIT_FAILURE);
    }

    // Inicializa o caminho atual como o diretório raiz da imagem
    strcpy(current_path, "/");

    ctrl_terminal();

    // Desmonta a imagem FAT32 antes de sair
    unmount_fat32(root);
    free(root);
    root = NULL;
}


void ctrl_terminal() {
    char input[MAX_COMMAND_SIZE]; // Buffer para entrada do usuário
    char command[32]; // Buffer para o comando
    char arg1[128], arg2[128]; // Buffers para argumentos
    int running = 1; // Flag para manter o terminal rodando

    while (running) {
        printf("FATal32> "); // Prompt do terminal
        fgets(input, sizeof(input), stdin); // Lê a entrada do usuário

        // Remove o caractere de nova linha do final da entrada
        input[strcspn(input, "\n")] = '\0';

        // Limpa os buffers
        memset(command, 0, sizeof(command));
        memset(arg1, 0, sizeof(arg1));
        memset(arg2, 0, sizeof(arg2));

        // Divide a entrada em comando e argumentos
        sscanf(input, "%s %s %s", command, arg1, arg2);

        // Verifica qual comando foi digitado e chama a função correspondente
        if(strcmp(command, "clear") == 0) {
            system("clear"); // limpar tela - limpar ache log?
        } else if(strcmp(command, "help") == 0) {
            print_opcoes(); // Mostra as opções
        } else if (strcmp(command, "extra") == 0) {
            print_extra(); // Mostra o extra
        } else if (strcmp(command, "info") == 0) {
            cmd_info();
        } else if (strcmp(command, "cluster") == 0) {
            if (arg1[0] != '\0') {
                cmd_cluster(atoi(arg1));
            } else {
                printf("Erro: O comando 'cluster' requer um número como argumento.\n");
            }
        } else if (strcmp(command, "pwd") == 0) {
            cmd_pwd();
        } else if (strcmp(command, "attr") == 0) {
            if (arg1[0] != '\0') {
                cmd_attr(arg1);
            } else {
                printf("Erro: O comando 'attr' requer o caminho de um arquivo ou diretório.\n");
            }
        } else if (strcmp(command, "cd") == 0) {
            if (arg1[0] != '\0') {
                cmd_cd(arg1);
            } else {
                printf("Erro: O comando 'cd' requer um caminho como argumento.\n");
            }
        } else if (strcmp(command, "touch") == 0) {
            if (arg1[0] != '\0') {
                cmd_touch(arg1);
            } else {
                printf("Erro: O comando 'touch' requer o nome de um arquivo como argumento.\n");
            }
        } else if (strcmp(command, "mkdir") == 0) {
            if (arg1[0] != '\0') {
                cmd_mkdir(arg1);
            } else {
                printf("Erro: O comando 'mkdir' requer o nome de um diretório como argumento.\n");
            }
        } else if (strcmp(command, "rm") == 0) {
            if (arg1[0] != '\0') {
                cmd_rm(arg1);
            } else {
                printf("Erro: O comando 'rm' requer o nome de um arquivo como argumento.\n");
            }
        } else if (strcmp(command, "rmdir") == 0) {
            if (arg1[0] != '\0') {
                cmd_rmdir(arg1);
            } else {
                printf("Erro: O comando 'rmdir' requer o nome de um diretório como argumento.\n");
            }
        } else if (strcmp(command, "cp") == 0) {
            if (arg1[0] != '\0' && arg2[0] != '\0') {
                cmd_cp(arg1, arg2);
            } else {
                printf("Erro: O comando 'cp' requer os caminhos de origem e destino.\n");
            }
        } else if (strcmp(command, "mv") == 0) {
            if (arg1[0] != '\0' && arg2[0] != '\0') {
                cmd_mv(arg1, arg2);
            } else {
                printf("Erro: O comando 'mv' requer os caminhos de origem e destino.\n");
            }
        } else if (strcmp(command, "rename") == 0) {
            if (arg1[0] != '\0' && arg2[0] != '\0') {
                cmd_rename(arg1, arg2);
            } else {
                printf("Erro: O comando 'rename' requer o nome antigo e o novo nome.\n");
            }
        } else if (strcmp(command, "ls") == 0) {
            cmd_ls();
        } else if (strcmp(command, "exit") == 0) {
            running = 0; // Encerra o terminal
        } else {
            printf("Comando desconhecido: '%s'. Digite um comando válido.\n", command);
        }
    }

    printf("Encerrando FATal32!\n");
}


const char* validate_arguments(int argc, char *argv[]) {
    if (argc == 2) {
        // Apenas nome da imagem fornecido, assume que está no diretório atual.
        if (access(argv[1], 0) == 0) { // Verifica se o arquivo existe
            return argv[1];
        } else {
            fprintf(stderr, "Erro: Arquivo %s não encontrado.\n", argv[1]);
            return NULL;
        }
    } else if (argc == 3 && strcmp(argv[1], "-r") == 0) {
        // Caminho fornecido com a flag -r.
        if (access(argv[2], F_OK) != -1) { // Verifica se o arquivo existe.
            return argv[2];
        } else {
            fprintf(stderr, "Erro: O arquivo especificado não existe: %s\n", argv[2]);
            return NULL;
        }
    } else {
        // Parâmetros inválidos.
        fprintf(stderr, "Uso: ./fatal32 <imagem_fat32.iso>\n");
        fprintf(stderr, "     ./fatal32 -r <local_do_arquivo/pasta/imagem_fat32.iso>\n");
        return NULL;
    }
}

