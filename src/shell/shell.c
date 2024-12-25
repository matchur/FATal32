#include "shell.h"
#include <stdio.h>
#include <string.h>

// Definir tamanho máximo de comando
#define MAX_COMMAND_SIZE 256

void start_shell() {
    char command[MAX_COMMAND_SIZE];
    int running = 1;

    system("clear");
    print_ascii_art();
    printf("Digite 'help' para ver os comandos disponíveis ou 'exit' para sair.\n");

    while (running) {
        // Exibir prompt
        
        printf("fatal32> ");
        fflush(stdout);

        // Ler comando do usuário
        if (!fgets(command, sizeof(command), stdin)) {
            perror("Erro ao ler o comando");
            break;
        }

        // Remover nova linha do final do comando
        command[strcspn(command, "\n")] = '\0';

        // Processar comando
        if (strcmp(command, "exit") == 0) {
            printf("Saindo do terminal...\n");
            running = 0;
        } else if (strcmp(command, "help") == 0) {
            print_menu(); // Função do módulo menu para exibir o menu
        } else if (strncmp(command, "fat", 3) == 0) {
            // Comando relacionado ao FAT32
            handle_fat_command(command); // Função específica para comandos FAT32
        } else if (strncmp(command, "cmd", 3) == 0) {
            // Comando relacionado ao módulo commands
            handle_cmd(command); // Função do módulo commands
        } else {
            printf("Comando não reconhecido: '%s'\n", command);
        }
    }
}
