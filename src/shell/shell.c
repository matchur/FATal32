#include "shell.h"
#include <stdio.h>
#include <string.h>

// Definir tamanho máximo de comando
#define MAX_COMMAND_SIZE 256

void start_shell() {
    system("clear");
    print_ascii_art();
    ctrl_terminal();
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

    printf("Encerrando FATal32! Bons sonhos!\n");
}